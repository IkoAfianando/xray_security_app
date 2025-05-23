from sqlalchemy import Column, Integer, String, DateTime, ForeignKey, Boolean, Float
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship
from datetime import datetime
from .database import Base

class Operator(Base):
    __tablename__ = "operators"

    id = Column(Integer, primary_key=True, index=True)
    name = Column(String, index=True)
    fingerprint_id = Column(Integer, unique=True)
    role = Column(String, default="operator")  # roles: operator, admin
    password_hash = Column(String, nullable=True)
    email = Column(String, unique=True, nullable=True)
    phone = Column(String, nullable=True)
    status = Column(String, default="Active")  # Active, Pending, Suspended
    created_at = Column(DateTime, default=datetime.utcnow)
    
    usage_logs = relationship("UsageLog", back_populates="operator")
    attendance_logs = relationship("AttendanceLog", back_populates="operator")

class UsageLog(Base):
    __tablename__ = "usage_logs"

    id = Column(Integer, primary_key=True, index=True)
    operator_id = Column(Integer, ForeignKey("operators.id"))
    activation_time = Column(DateTime, default=datetime.utcnow)
    operational_duration = Column(Integer)
    error_log = Column(String, nullable=True)
    operator = relationship("Operator", back_populates="usage_logs")

class AttendanceLog(Base):
    __tablename__ = "attendance_logs"
    
    id = Column(Integer, primary_key=True, index=True)
    operator_id = Column(Integer, ForeignKey("operators.id"))
    fingerprint_id = Column(Integer)
    timestamp = Column(DateTime, default=datetime.utcnow)
    action = Column(String)  # "login" or "logout"
    status = Column(String, default="success")  # success, failed
    
    operator = relationship("Operator", back_populates="attendance_logs")


class TokenBlacklist(Base):
    __tablename__ = "token_blacklist"
    
    id = Column(Integer, primary_key=True, index=True)
    token = Column(String, unique=True, index=True)
    blacklisted_at = Column(DateTime, default=datetime.utcnow)
    expires_at = Column(DateTime)  # Untuk cleanup otomatis