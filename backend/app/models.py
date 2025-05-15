from sqlalchemy import Column, Integer, String, DateTime, ForeignKey
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
    password_hash = Column(String, nullable=True)  # store hashed password
    usage_logs = relationship("UsageLog", back_populates="operator")

class UsageLog(Base):
    __tablename__ = "usage_logs"

    id = Column(Integer, primary_key=True, index=True)
    operator_id = Column(Integer, ForeignKey("operators.id"))
    activation_time = Column(DateTime, default=datetime.utcnow)
    operational_duration = Column(Integer)
    error_log = Column(String, nullable=True)  # To log any errors during operation
    operator = relationship("Operator", back_populates="usage_logs")
