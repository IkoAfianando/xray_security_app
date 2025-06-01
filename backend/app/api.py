import uuid
from fastapi import APIRouter, Depends, HTTPException, status, Query
from pydantic import BaseModel
from sqlalchemy.orm import Session, joinedload
from app import models, schemas
from app.database import get_db
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from passlib.context import CryptContext
from datetime import datetime, timedelta
import jwt
from typing import List, Optional

SECRET_KEY = "REAL_MADRID_THE_BEST_CLUB_IN_THE_WORLD"
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 30

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="token")

router = APIRouter()

def verify_password(plain_password, hashed_password):
    return pwd_context.verify(plain_password, hashed_password)

def get_password_hash(password):
    return pwd_context.hash(password)

def create_access_token(data: dict, expires_delta: timedelta | None = None):
    to_encode = data.copy()
    if expires_delta:
        expire = datetime.utcnow() + expires_delta
    else:
        expire = datetime.utcnow() + timedelta(minutes=15)
    to_encode.update({"exp": expire})
    encoded_jwt = jwt.encode(to_encode, SECRET_KEY, algorithm=ALGORITHM)
    return encoded_jwt

def get_user(db, fingerprint_id: int):
    return db.query(models.Operator).filter(models.Operator.fingerprint_id == fingerprint_id).first()

def authenticate_user(db, fingerprint_id: int, password: str):
    user = get_user(db, fingerprint_id)
    if not user:
        return False
    if not verify_password(password, user.password_hash):
        return False
    return user

def is_token_blacklisted(token: str, db: Session):
    blacklisted = db.query(models.TokenBlacklist).filter(models.TokenBlacklist.token == token).first()
    return blacklisted is not None

def add_token_to_blacklist(token: str, db: Session):
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        exp_timestamp = payload.get("exp")
        expires_at = datetime.fromtimestamp(exp_timestamp) if exp_timestamp else datetime.utcnow() + timedelta(hours=1)
        
        blacklisted_token = models.TokenBlacklist(
            token=token,
            expires_at=expires_at
        )
        db.add(blacklisted_token)
        db.commit()
        return True
    except Exception as e:
        print(f"Error blacklisting token: {e}")
        return False

@router.post("/token")
async def login_for_access_token(form_data: OAuth2PasswordRequestForm = Depends(), db: Session = Depends(get_db)):
    user = authenticate_user(db, int(form_data.username), form_data.password)
    if not user:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Incorrect fingerprint ID or password")
    
    if user.role != "admin":
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Admin access required")
    
    access_token_expires = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(data={"sub": str(user.fingerprint_id)}, expires_delta=access_token_expires)
    
    return {
        "access_token": access_token, 
        "token_type": "bearer", 
        "user": {
            "id": user.id,
            "name": user.name,
            "fingerprint_id": user.fingerprint_id,
            "role": user.role,
            "email": user.email
        }
    }

def get_current_user(token: str = Depends(oauth2_scheme), db: Session = Depends(get_db)):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )
    
    if is_token_blacklisted(token, db):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Token has been revoked",
            headers={"WWW-Authenticate": "Bearer"},
        )
    
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        fingerprint_id: str = payload.get("sub")
        if fingerprint_id is None:
            raise credentials_exception
    except jwt.PyJWTError:
        raise credentials_exception
    
    user = get_user(db, int(fingerprint_id))
    if user is None:
        raise credentials_exception
    return user

def require_admin(current_user: models.Operator = Depends(get_current_user)):
    if current_user.role != "admin":
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Admin access required")
    return current_user

# Operators/Users endpoints
@router.post("/operators/", response_model=schemas.OperatorResponse)
def create_operator(operator: schemas.OperatorCreate, db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    # Check if fingerprint_id already exists
    existing_operator = db.query(models.Operator).filter(models.Operator.fingerprint_id == operator.fingerprint_id).first()
    if existing_operator:
        raise HTTPException(status_code=400, detail="Fingerprint ID already registered")
    
    # Check if email already exists
    if operator.email:
        existing_email = db.query(models.Operator).filter(models.Operator.email == operator.email).first()
        if existing_email:
            raise HTTPException(status_code=400, detail="Email already registered")
    # generated_uuid = str(uuid.uuid4())
    hashed_password = get_password_hash(operator.password)
    db_operator = models.Operator(        
        name=operator.name,
        fingerprint_id=operator.fingerprint_id,
        role=operator.role,
        email=operator.email,
        phone=operator.phone,
        status=operator.status,
        password_hash=hashed_password
    )
    db.add(db_operator)
    db.commit()
    db.refresh(db_operator)
    return db_operator

@router.get("/operators/", response_model=List[schemas.OperatorResponse])
def get_all_operators(
    skip: int = 0, 
    limit: int = 100, 
    search: Optional[str] = Query(None, description="Search by name or email"),
    status_filter: Optional[str] = Query(None, description="Filter by status"),
    db: Session = Depends(get_db), 
    current_user: models.Operator = Depends(require_admin)
):
    query = db.query(models.Operator)
    print("search", search or "None")
    print("status_filter", status_filter or "None")
    if search:
        query = query.filter(
            (models.Operator.name.ilike(f"%{search}%")) |
            (models.Operator.email.ilike(f"%{search}%"))
        )
    
    if status_filter:
        query = query.filter(models.Operator.status == status_filter)
    
    operators = query.offset(skip).limit(limit).all()
    return operators

@router.get("/operators/{operator_id}", response_model=schemas.OperatorResponse)
def get_operator_by_id(operator_id: int, db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    operator = db.query(models.Operator).filter(models.Operator.id == operator_id).first()
    if operator is None:
        raise HTTPException(status_code=404, detail="Operator not found")
    return operator

@router.put("/operators/{operator_id}", response_model=schemas.OperatorResponse)
def update_operator(operator_id: int, operator_update: schemas.OperatorUpdate, db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    operator = db.query(models.Operator).filter(models.Operator.id == operator_id).first()
    if operator is None:
        raise HTTPException(status_code=404, detail="Operator not found")
    
    update_data = operator_update.dict(exclude_unset=True)
    for field, value in update_data.items():
        setattr(operator, field, value)
    
    db.commit()
    db.refresh(operator)
    return operator

@router.delete("/operators/{operator_id}")
def delete_operator(operator_id: int, db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    operator = db.query(models.Operator).filter(models.Operator.id == operator_id).first()
    if operator is None:
        raise HTTPException(status_code=404, detail="Operator not found")
    
    db.delete(operator)
    db.commit()
    return {"message": "Operator deleted successfully"}

@router.get("/operators/me", response_model=schemas.OperatorResponse)
def read_operators_me(current_user: models.Operator = Depends(get_current_user)):
    return current_user

# Logout endpoint
@router.post("/logout", response_model=schemas.LogoutResponse)
async def logout(token: str = Depends(oauth2_scheme), db: Session = Depends(get_db)):
    try:
        success = add_token_to_blacklist(token, db)
        
        if success:
            return schemas.LogoutResponse(
                message="Successfully logged out",
                success=True
            )
        else:
            raise HTTPException(
                status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                detail="Failed to logout"
            )
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Logout failed"
        )

# Attendance endpoints (untuk ESP32)
@router.post("/attendance/", response_model=schemas.AttendanceResponse)
def record_attendance(fingerprint_data: dict, db: Session = Depends(get_db)):
    fingerprint_id = fingerprint_data.get("FingerID")
    
    if not fingerprint_id:
        raise HTTPException(status_code=400, detail="FingerID required")
    
    operator = db.query(models.Operator).filter(models.Operator.fingerprint_id == fingerprint_id).first()
    
    if not operator:
        attendance_log = models.AttendanceLog(
            fingerprint_id=fingerprint_id,
            action="unknown",
            status="failed"
        )
        db.add(attendance_log)
        db.commit()
        raise HTTPException(status_code=404, detail="Operator not found")
    
    last_attendance = db.query(models.AttendanceLog).filter(
        models.AttendanceLog.operator_id == operator.id
    ).order_by(models.AttendanceLog.timestamp.desc()).first()
    
    action = "login"
    if last_attendance and last_attendance.action == "login":
        action = "logout"
    
    attendance_log = models.AttendanceLog(
        operator_id=operator.id,
        fingerprint_id=fingerprint_id,
        action=action,
        status="success"
    )
    db.add(attendance_log)
    db.commit()
    
    response_message = f"{action}{operator.name}"
    
    return schemas.AttendanceResponse(
        message=response_message,
        user_name=operator.name,
        action=action
    )

# Usage logs endpoints
@router.post("/usage_logs/", response_model=schemas.UsageLog)
def create_usage_log(usage_log: schemas.UsageLogCreate, db: Session = Depends(get_db), current_user: models.Operator = Depends(get_current_user)):
    db_usage_log = models.UsageLog(**usage_log.dict())
    db.add(db_usage_log)
    db.commit()
    db.refresh(db_usage_log)
    return db_usage_log

@router.get("/usage_logs/", response_model=List[schemas.UsageLog])
def get_usage_logs(skip: int = 0, limit: int = 100, db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    logs = db.query(models.UsageLog).join(models.Operator).options(joinedload(models.UsageLog.operator)).offset(skip).limit(limit).all()
    return logs

@router.get("/attendance_logs/", response_model=List[schemas.AttendanceLog])
def get_attendance_logs(skip: int = 0, limit: int = 100, db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    logs = db.query(models.AttendanceLog).join(models.Operator).options(joinedload(models.AttendanceLog.operator)).offset(skip).limit(limit).all()
    return logs

# Dashboard stats
@router.get("/dashboard/stats")
def get_dashboard_stats(db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    total_operators = db.query(models.Operator).count()
    active_operators = db.query(models.Operator).filter(models.Operator.status == "Active").count()
    today_attendance = db.query(models.AttendanceLog).filter(
        models.AttendanceLog.timestamp >= datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    ).count()
    pending_operators = db.query(models.Operator).filter(models.Operator.status == "Pending").count()
    
    return {
        "total_operators": total_operators,
        "active_operators": active_operators,
        "today_attendance": today_attendance,
        "pending_operators": pending_operators
    }

class FingerprintEnrollRequest(BaseModel):
    status: str  
    fingerprint_id_real: str

class FingerprintLoginRequest(BaseModel):    
    confidence: int
    fingerprint_id_real: str


@router.post('/fingerprint_login')
def fingerprint_login(fingerprint_loging_request: FingerprintLoginRequest, db: Session = Depends(get_db)):
    fingerprint_id_real = fingerprint_loging_request.fingerprint_id_real    
    confidence = fingerprint_loging_request.confidence
    operator = db.query(models.Operator).filter(models.Operator.fingerprint_id_real == fingerprint_id_real).first()
    if not operator:
        operator = db.query(models.Operator).filter(models.Operator.fingerprint_id_real == fingerprint_id_real).first()
        if not operator:
            raise HTTPException(status_code=404, detail="Operator not found")
        else:
            operator.fingerprint_id_real = fingerprint_id_real            
            db.commit()
            db.refresh(operator)
            return operator
    else:
        operator.fingerprint_id_real = fingerprint_id_real
        db.commit()
        db.refresh(operator)
    return operator



# enroll mean is register to database fingerprint_id_real and status, default role is operator
@router.post('/fingerprint_enroll')
def fingerprint_enroll(fingerprint_enroll_request: FingerprintEnrollRequest, db: Session = Depends(get_db)):
    fingerprint_id_real = fingerprint_enroll_request.fingerprint_id_real
    status = fingerprint_enroll_request.status
    operator = models.Operator(
        fingerprint_id=int(fingerprint_id_real),
        fingerprint_id_real=fingerprint_id_real,
        status=status,
        role="operator",
        password_hash="$2a$12$x87AozIUJswVGygCcGW8rOYiVAxyXq9iLIY/YF/V8notCGtgecg2m"
    )
    db.add(operator)
    db.commit()
    return operator



# @router.post('/fingerprint_enroll')
# def fingerprint_enroll(fingerprint_enroll_request: FingerprintEnrollRequest, db: Session = Depends(get_db)):
#     fingerprint_id_real = fingerprint_enroll_request.fingerprint_id_real
#     status = fingerprint_enroll_request.status
#     print("status", status)
#     print("fingerprint_id_real", fingerprint_id_real)
#     if status == "enrolled":
#         operator = db.query(models.Operator).filter(models.Operator.fingerprint_id_real == fingerprint_id_real).first()
#         if not operator:
#             raise HTTPException(status_code=404, detail="Operator not found")
#         else:
#             operator.fingerprint_id_real = fingerprint_id_real
#             operator.fingerprint_id = int(fingerprint_id_real)
#             operator.status = "Active"
#             db.commit()
#             db.refresh(operator)
#     else:
#         operator = db.query(models.Operator).filter(models.Operator.fingerprint_id_real == fingerprint_id_real).first()
#         if not operator:
#             raise HTTPException(status_code=404, detail="Operator not found")
#         else:
#             operator.status = "Pending"
#             db.commit()
#             db.refresh(operator)
#     return operator