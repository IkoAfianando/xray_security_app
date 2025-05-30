"""new enhancment

Revision ID: de51a1c5c3a4
Revises: 009bc6b6827f
Create Date: 2025-05-23 17:11:05.240470

"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision: str = 'de51a1c5c3a4'
down_revision: Union[str, None] = '009bc6b6827f'
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    """Upgrade schema."""
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_table('users')
    op.add_column('operators', sa.Column('email', sa.String(), nullable=True))
    op.add_column('operators', sa.Column('phone', sa.String(), nullable=True))
    op.add_column('operators', sa.Column('status', sa.String(), nullable=True))
    op.add_column('operators', sa.Column('created_at', sa.DateTime(), nullable=True))
    op.create_unique_constraint(None, 'operators', ['email'])
    # ### end Alembic commands ###


def downgrade() -> None:
    """Downgrade schema."""
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_constraint(None, 'operators', type_='unique')
    op.drop_column('operators', 'created_at')
    op.drop_column('operators', 'status')
    op.drop_column('operators', 'phone')
    op.drop_column('operators', 'email')
    op.create_table('users',
    sa.Column('id', sa.INTEGER(), autoincrement=True, nullable=False),
    sa.Column('username', sa.VARCHAR(length=255), autoincrement=False, nullable=True),
    sa.Column('serial_number', sa.VARCHAR(length=255), autoincrement=False, nullable=True),
    sa.Column('fingerprint_id', sa.INTEGER(), autoincrement=False, nullable=True),
    sa.Column('fingerprint_select', sa.VARCHAR(length=100), autoincrement=False, nullable=True),
    sa.PrimaryKeyConstraint('id', name='users_pkey')
    )
    # ### end Alembic commands ###
