from datetime import datetime
from flask_login import UserMixin
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy_utils import ChoiceType

db = SQLAlchemy()

#guarda la informacion de los dispositivos IoT
class Dispositivos(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    id_modelo = db.Column(db.String(length=50), unique=True)
    name = db.Column(db.String(length=150))

    def __repr__(self):
        return f"IoT:{self.name}-{self.id_modelo}"

#guarda la informacion de los usuarios
# este usuario es el dueño del IoT
class Usuarios(db.Model, UserMixin):
    id = db.Column(db.Integer, primary_key=True)
    email = db.Column(db.String(length=150), unique=True)
    nombre = db.Column(db.String(length=150))
    apellido = db.Column(db.String(length=150))
    telefono = db.Column(db.String(length=150))
    password_hash = db.Column(db.String(200))
    
    def __repr__(self):
        return f"U: {self.email}"

#modelo para unir dispositivos y usuarios   
ESTADO_OPERACIONAL = [
    ('Desbloqueado', 'Desbloqueado'),
    ('Bloqueado', 'Bloqueado'),
    ('Err', 'Error')
]
class iot_usuario(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    iot_id = db.Column(db.Integer, db.ForeignKey("dispositivos.id"))
    usuario_id = db.Column(db.Integer, db.ForeignKey("usuarios.id"))
    codigo = db.Column(db.String(length=150))
    encendido = db.Column(db.Boolean, default=False) 
    estado = db.Column(ChoiceType(ESTADO_OPERACIONAL))
    alias = db.Column(db.String(150))
    bateria = db.Column(db.Integer)
    intentos = db.Column(db.Integer)
    usuario = db.relationship('Usuarios')
    iot = db.relationship('Dispositivos')
    __table_args__ = (
        db.UniqueConstraint('iot_id', 'usuario_id', name='uq_iot_usuario'),
    )

    def __repr__(self):
        return f"{self.iot_id}::{self.usuario_id}"

#modelo para almacener datos de uso del dispositivo
ACCESO = [
    ("ACK", "Aceptado"),
    ("NAK", "Denegado")
]
ACCION= [
    ("BLCK", "Bloquear"),
    ("DBLCK", "Desbloquear")
]
class iotlogs(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    iot_id = db.Column(db.Integer, db.ForeignKey("iot_usuario.id"))
    instante = db.Column(db.DateTime, default=datetime.now) #YYYY-MM-DD HH:MM:SS:MS
    acceso = db.Column(ChoiceType(ACCESO))
    accion = db.Column(ChoiceType(ACCION))

    iot = db.relationship('iot_usuario')

    

def crear_bd(app):
    db.init_app(app)
    with app.app_context():
        db.create_all()