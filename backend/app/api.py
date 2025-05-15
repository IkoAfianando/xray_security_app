from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from app import models, schemas
from app.database import get_db
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from passlib.context import CryptContext
from datetime import datetime, timedelta
import jwt

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

@router.post("/token")
async def login_for_access_token(form_data: OAuth2PasswordRequestForm = Depends(), db: Session = Depends(get_db)):
    user = authenticate_user(db, int(form_data.username), form_data.password)
    if not user:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Incorrect fingerprint ID or password")
    access_token_expires = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(data={"sub": str(user.fingerprint_id)}, expires_delta=access_token_expires)
    return {"access_token": access_token, "token_type": "bearer"}

def get_current_user(token: str = Depends(oauth2_scheme), db: Session = Depends(get_db)):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
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

@router.post("/operators/", response_model=schemas.Operator)
def create_operator(operator: schemas.OperatorCreate, db: Session = Depends(get_db)):
    hashed_password = get_password_hash(operator.password)
    db_operator = models.Operator(
        name=operator.name,
        fingerprint_id=operator.fingerprint_id,
        role=operator.role,
        password_hash=hashed_password
    )
    db.add(db_operator)
    db.commit()
    db.refresh(db_operator)
    return db_operator

@router.get("/operators/me", response_model=schemas.Operator)
def read_operators_me(current_user: models.Operator = Depends(get_current_user)):
    return current_user

@router.get("/operators/{fingerprint_id}", response_model=schemas.Operator)
def get_operator(fingerprint_id: int, db: Session = Depends(get_db), current_user: models.Operator = Depends(get_current_user)):
    operator = db.query(models.Operator).filter(models.Operator.fingerprint_id == fingerprint_id).first()
    if operator is None:
        raise HTTPException(status_code=404, detail="Operator not found")
    return operator

@router.post("/usage_logs/", response_model=schemas.UsageLog)
def create_usage_log(usage_log: schemas.UsageLogCreate, db: Session = Depends(get_db), current_user: models.Operator = Depends(get_current_user)):
    db_usage_log = models.UsageLog(**usage_log.dict())
    db.add(db_usage_log)
    db.commit()
    db.refresh(db_usage_log)
    return db_usage_log

@router.get("/usage_logs/")
def get_usage_logs(skip: int = 0, limit: int = 100, db: Session = Depends(get_db), current_user: models.Operator = Depends(get_current_user)):
    logs = db.query(models.UsageLog).offset(skip).limit(limit).all()
    return logs
