---

# X-Ray Security App - Backend

### Deskripsi Proyek

Ini adalah backend REST API untuk aplikasi X-Ray Security, dibangun menggunakan **Python** dan **FastAPI**. API ini menangani autentikasi operator, pencatatan penggunaan, dan berbagai endpoint terkait.

---

### Persyaratan Sistem

- **Python 3.10+**
- **PostgreSQL Database**  
  Jika belum memiliki PostgreSQL, silakan download dan install:
  - [Windows Download](https://www.postgresql.org/download/windows/)
  - [Linux Download](https://www.postgresql.org/download/linux/)
  - [macOS Download](https://www.postgresql.org/download/macosx/)

---

### Cara Menjalankan Project dari Awal

#### 1. Clone Repository ini

```bash
git clone 
cd backend
```

#### 2. Buat dan Aktifkan Virtual Environment

```bash
python -m venv venv
source venv/bin/activate      # Linux/macOS
venv\Scripts\activate         # Windows
```

#### 3. Install Dependencies

```bash
pip install -r requirements.txt
```

#### 4. Konfigurasi Environment Variables  
Buat file `.env` di root folder (jika belum ada):

```env
DATABASE_URL=postgresql://root:secret@localhost/xray
```
> ⚠️ Ganti `root`, `secret`, dan `xray` sesuai konfigurasi username, password, dan database PostgreSQL kamu.

#### 5. Setup Database

- **Buat database di PostgreSQL** dengan nama yang sesuai `.env` (misalnya `xray`):

```sql
-- Masuk ke psql, lalu:
CREATE DATABASE xray;
```

#### 6. Jalankan Database Migration (optional, jika pakai Alembic)

```bash
alembic upgrade head
```

#### 7. Jalankan Backend

```bash
uvicorn main:app --reload
```

Akses API di: [http://localhost:8000](http://localhost:8000)

---

### Struktur Folder (Ringkasan)

```
backend/
│
├── main.py
├── .env
├── alembic.ini
├── app/
│   ├── __init__.py
│   ├── models.py
│   ├── schemas.py
│   ├── database.py
│   └── api.py
└── alembic/
```

---

### Disclaimer

- Pastikan PostgreSQL sudah ter-install dan berjalan sebelum menjalankan project.
- Atur kredensial database dengan benar di `.env`!
- Untuk dokumentasi lebih lanjut tentang FastAPI: https://fastapi.tiangolo.com/

---

## 🇬🇧 English README

### Project Description

This is the backend REST API for the X-Ray Security application, built with **Python** and **FastAPI**. The API handles operator authentication, usage logging, and various related endpoints.

---

### System Requirements

- **Python 3.10+**
- **PostgreSQL Database**  
  If you don't have PostgreSQL, please download and install from:
  - [Download for Windows](https://www.postgresql.org/download/windows/)
  - [Download for Linux](https://www.postgresql.org/download/linux/)
  - [Download for macOS](https://www.postgresql.org/download/macosx/)

---

### Quick Start Guide (from Scratch)

#### 1. Clone This Repository

```bash
git clone https://github.com/IkoAfianando/xray_security_app.git
cd backend
```

#### 2. Create & Activate Virtual Environment

```bash
python -m venv venv
source venv/bin/activate      # Linux/macOS
venv\Scripts\activate         # Windows
```

#### 3. Install Dependencies

```bash
pip install -r requirements.txt
```

#### 4. Configure Environment Variables  
Create a `.env` file in the root (if not exists):

```env
DATABASE_URL=postgresql://root:secret@localhost/xray
```
> ⚠️ Adjust `root`, `secret`, and `xray` to match your PostgreSQL credentials and target database.

#### 5. Setup Database

- **Create the database in PostgreSQL** as specified in `.env` (e.g., `xray`):

```sql
-- In psql shell:
CREATE DATABASE xray;
```

#### 6. Run Database Migrations (if using Alembic)

```bash
alembic upgrade head
```

#### 7. Start the Backend Server

```bash
uvicorn main:app --reload
```

The API should be available at: [http://localhost:8000](http://localhost:8000)

---

### Folder Structure (Summary)

```
backend/
│
├── main.py
├── .env
├── alembic.ini
├── app/
│   ├── __init__.py
│   ├── models.py
│   ├── schemas.py
│   ├── database.py
│   └── api.py
└── alembic/
```

---

### Disclaimer

- Make sure PostgreSQL is installed and running before running the project.
- Double-check your database credentials in `.env`!
- For more FastAPI documentation: https://fastapi.tiangolo.com/

## License

This project is open-source and available under the MIT License.

---