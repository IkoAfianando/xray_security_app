# backend/main.py

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

# Now importing directly from the 'app' package,
# as these are re-exported by app/__init__.py
from app import router, engine, Base

# Create database tables. This line needs 'Base' and 'engine'.
Base.metadata.create_all(bind=engine)

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000"], # Or your frontend origin
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include the API router
app.include_router(router)

# You might add other application setup or root endpoints here if needed
@app.get("/")
async def root():
    return {"message": "Welcome to the X-Ray Security App API"}

