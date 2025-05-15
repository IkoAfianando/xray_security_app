from pydantic import BaseModel
from datetime import datetime
from typing import Optional

class OperatorBase(BaseModel):
    name: str
    fingerprint_id: int
    role: Optional[str] = "operator"

class OperatorCreate(OperatorBase):
    password: str  # password on creation

class Operator(OperatorBase):
    id: int

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

    class Config:
        orm_mode = True
