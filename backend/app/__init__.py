# backend/app/__init__.py

# Re-export key components from submodules

# From database.py  
from .database import engine, Base, get_db, SessionLocal

# From models.py
# (Import specific models you want to make available at the app package level)
from .models import Operator, UsageLog

# From schemas.py
# (Import specific Pydantic schemas, aliasing if names might clash, e.g., with models)
from .schemas import (
    Operator as OperatorSchema,
    OperatorCreate,
    UsageLog as UsageLogSchema,
    UsageLogCreate
)

# From api.py
from .api import router

# Optionally, define __all__ to specify what is exported with 'from app import *'
# This is good practice for libraries, but less critical for application internal packages.
__all__ = [
    "engine",
    "Base",
    "get_db",
    "SessionLocal",
    "Operator",
    "UsageLog",
    "OperatorSchema",
    "OperatorCreate",
    "UsageLogSchema",
    "UsageLogCreate",
    "router",
]
