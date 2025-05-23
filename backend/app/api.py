from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session, joinedload
from app import models, schemas
from app.database import get_db
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from passlib.context import CryptContext
from datetime import datetime, timedelta
import jwt
from typing import List

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
    """Check if token is in blacklist"""
    blacklisted = db.query(models.TokenBlacklist).filter(models.TokenBlacklist.token == token).first()
    return blacklisted is not None

def add_token_to_blacklist(token: str, db: Session):
    """Add token to blacklist"""
    try:
        # Decode token to get expiration time
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
    
    # Check if user has admin role for dashboard access
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
            "role": user.role
        }
    }

def get_current_user(token: str = Depends(oauth2_scheme), db: Session = Depends(get_db)):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )
    
    # Check if token is blacklisted
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

# Logout endpoint
@router.post("/logout", response_model=schemas.LogoutResponse)
async def logout(token: str = Depends(oauth2_scheme), db: Session = Depends(get_db)):
    """
    Logout user by blacklisting the current token
    """
    try:
        # Add token to blacklist
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

# Cleanup expired blacklisted tokens (optional endpoint for maintenance)
@router.delete("/cleanup-tokens")
async def cleanup_expired_tokens(db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    """
    Remove expired tokens from blacklist
    """
    try:
        expired_tokens = db.query(models.TokenBlacklist).filter(
            models.TokenBlacklist.expires_at < datetime.utcnow()
        ).all()
        
        count = len(expired_tokens)
        
        for token in expired_tokens:
            db.delete(token)
        
        db.commit()
        
        return {"message": f"Cleaned up {count} expired tokens"}
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Cleanup failed"
        )

# Rest of your existing endpoints...
@router.post("/operators/", response_model=schemas.Operator)
def create_operator(operator: schemas.OperatorCreate, db: Session = Depends(get_db)):
    hashed_password = get_password_hash(operator.password)
    db_operator = models.Operator(
        name=operator.name,
        fingerprint_id=operator.fingerprint_id,
        role=operator.role,
        email=operator.email,
        phone=operator.phone,
        password_hash=hashed_password
    )
    db.add(db_operator)
    db.commit()
    db.refresh(db_operator)
    return db_operator

@router.get("/operators/me", response_model=schemas.Operator)
def read_operators_me(current_user: models.Operator = Depends(get_current_user)):
    return current_user


@router.get("/operators/", response_model=List[schemas.Operator])
def get_all_operators(db: Session = Depends(get_db), current_user: models.Operator = Depends(require_admin)):
    operators = db.query(models.Operator).all()
    return operators

@router.get("/operators/{fingerprint_id}", response_model=schemas.Operator)
def get_operator(fingerprint_id: int, db: Session = Depends(get_db), current_user: models.Operator = Depends(get_current_user)):
    operator = db.query(models.Operator).filter(models.Operator.fingerprint_id == fingerprint_id).first()
    if operator is None:
        raise HTTPException(status_code=404, detail="Operator not found")
    return operator

# Attendance endpoints (untuk ESP32)
@router.post("/attendance/", response_model=schemas.AttendanceResponse)
def record_attendance(fingerprint_data: dict, db: Session = Depends(get_db)):
    fingerprint_id = fingerprint_data.get("FingerID")
    
    if not fingerprint_id:
        raise HTTPException(status_code=400, detail="FingerID required")
    
    # Cari operator berdasarkan fingerprint_id
    operator = db.query(models.Operator).filter(models.Operator.fingerprint_id == fingerprint_id).first()
    
    if not operator:
        # Log attendance failed
        attendance_log = models.AttendanceLog(
            fingerprint_id=fingerprint_id,
            action="unknown",
            status="failed"
        )
        db.add(attendance_log)
        db.commit()
        raise HTTPException(status_code=404, detail="Operator not found")
    
    # Cek attendance terakhir untuk menentukan login/logout
    last_attendance = db.query(models.AttendanceLog).filter(
        models.AttendanceLog.operator_id == operator.id
    ).order_by(models.AttendanceLog.timestamp.desc()).first()
    
    action = "login"
    if last_attendance and last_attendance.action == "login":
        action = "logout"
    
    # Simpan attendance log
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
    
    return {
        "total_operators": total_operators,
        "active_operators": active_operators,
        "today_attendance": today_attendance,
        "pending_operators": total_operators - active_operators
    }
