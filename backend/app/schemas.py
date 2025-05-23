from pydantic import BaseModel, EmailStr
from datetime import datetime
from typing import Optional, List

class OperatorBase(BaseModel):
    name: str
    fingerprint_id: int
    role: Optional[str] = "operator"
    email: Optional[EmailStr] = None
    phone: Optional[str] = None
    status: Optional[str] = "Active"

class OperatorCreate(OperatorBase):
    password: str

class OperatorUpdate(BaseModel):
    name: Optional[str] = None
    email: Optional[EmailStr] = None
    phone: Optional[str] = None
    status: Optional[str] = None
    role: Optional[str] = None

class Operator(OperatorBase):
    id: int
    created_at: datetime

    class Config:
        orm_mode = True

class OperatorResponse(BaseModel):
    id: int
    name: str
    fingerprint_id: int
    role: str
    email: Optional[str] = None
    phone: Optional[str] = None
    status: str
    created_at: datetime

    class Config:
        orm_mode = True

class UsageLogBase(BaseModel):
    operational_duration: int
    error_log: Optional[str] = None

class UsageLogCreate(UsageLogBase):
    operator_id: int

class UsageLog(UsageLogBase):
    id: int
    operator_id: int
    activation_time: datetime
    operator: Optional[Operator] = None

    class Config:
        orm_mode = True

class AttendanceLogBase(BaseModel):
    fingerprint_id: int
    action: str
    status: Optional[str] = "success"

class AttendanceLogCreate(AttendanceLogBase):
    pass

class AttendanceLog(AttendanceLogBase):
    id: int
    operator_id: Optional[int] = None
    timestamp: datetime
    operator: Optional[Operator] = None

    class Config:
        orm_mode = True

class AttendanceResponse(BaseModel):
    message: str
    user_name: Optional[str] = None
    action: str

class LogoutResponse(BaseModel):
    message: str
    success: bool
