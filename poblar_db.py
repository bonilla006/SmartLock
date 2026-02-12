from app import app
from models import *
from datetime import datetime, timedelta
import random

def poblar_tablas():
    with app.app_context():
        print("Poblando tablas con datos")
        dispositivos = [
            Dispositivos(
                id_modelo="BFUA-6044",
                name="GoodLock", 
            ),
            Dispositivos(
                id_modelo="HKFA-3040", 
                name="SeC2000",
            ),
            Dispositivos(
                id_modelo="GEWF-6738",
                name="DobermanLock",
            ),
            Dispositivos(
                id_modelo="AAAA-0000",
                name="SmartLock2001",
            )
        ]
        
        for dispositivo in dispositivos:
            db.session.add(dispositivo)
        db.session.commit()
        print(f"{len(dispositivos)} dispositivos creados")
        
        # 2. Poblar Usuarios
        usuarios = [
            Usuarios(
                email="ana.garcia@example.com",
                nombre="Ana",
                apellido="García",
                telefono="+17879244706",
                password_hash="scrypt:32768:8:1$BBGFaOk5iORnOZpN$95407822abca166a6d0283a24459c3ad6b785d78e7721f080bd473d184a744b96376439819e414ce66891adf54a827f642fd6a4131a553df1558a216ae38a8e9" #pizza
            ),
            Usuarios(
                email="carlos.lopez@example.com", 
                nombre="Carlos",
                apellido="López",
                telefono="+17879339771",
                password_hash="scrypt:32768:8:1$yxygOa30qdx59rpg$908161939fd54cc51e7e07501421af380dfe874b67a49c9962871c3be1b5dd6ebcb617dfc9213241816e35984a424858b149d35398e0282c474510509df885cd" #hola
            ),
            Usuarios(
                email="maria.rodriguez@example.com",
                nombre="María",
                apellido="Rodríguez", 
                telefono="+19398141712",
                password_hash="scrypt:32768:8:1$noesunhash"
            )
        ]
        
        for usuario in usuarios:
            db.session.add(usuario)
        db.session.commit()
        print(f"{len(usuarios)} usuarios creados")
        
        # 3. Poblar Relaciones iot_usuario
        # Obtener IDs de dispositivos y usuarios recién creados
        dispositivo1 = Dispositivos.query.filter_by(id_modelo="BFUA-6044").first()
        dispositivo2 = Dispositivos.query.filter_by(id_modelo="HKFA-3040").first()
        usuario1 = Usuarios.query.filter_by(email="ana.garcia@example.com").first()
        usuario2 = Usuarios.query.filter_by(email="carlos.lopez@example.com").first()
        
        relaciones = [
            iot_usuario(
                iot_id=dispositivo1.id, 
                usuario_id=usuario1.id, 
                codigo="8c34f8ce0a50e63100f1681cd5597bf037ba20fe4298d78bfc14f146ef9bca85",
                encendido=True,
                estado="Bloqueado", 
                alias="Puerta Principal",
                bateria=89,
                intentos=0
            ),
            iot_usuario(
                iot_id=dispositivo2.id, 
                usuario_id=usuario1.id, 
                codigo="58948749", 
                encendido=True,
                estado="Bloqueado",
                alias="Patio",
                bateria=90,
                intentos=0  
            ),
            iot_usuario(
                iot_id=dispositivo1.id, 
                usuario_id=usuario2.id, 
                codigo="26284686",
                encendido=True,
                estado="Bloqueado", 
                alias="Oficina de Jorge",
                bateria=10,
                intentos=0
            ),
        ]
        
        for relacion in relaciones:
            db.session.add(relacion)
        db.session.commit()
        print(f"{len(relaciones)} relaciones creadas")

        # 4. Poblar iotlogs con datos de ejemplo
        print("Poblando tabla iotlogs...")
        
        # Obtener todas las relaciones iot_usuario
        relaciones_iot = iot_usuario.query.all()
        
        logs = []
        
        # Generar logs para los últimos 30 días
        for i in range(25):  # Crear 100 eventos de ejemplo
            # Seleccionar una relación aleatoria
            relacion = random.choice(relaciones_iot)
            
            # Fecha aleatoria en los últimos 30 días
            dias_atras = random.randint(0, 30)
            horas_atras = random.randint(0, 23)
            minutos_atras = random.randint(0, 59)
            
            instante = datetime.now() - timedelta(
                days=dias_atras, 
                hours=horas_atras, 
                minutes=minutos_atras
            )
            
            # 85% de probabilidad de acceso aceptado, 15% denegado
            acceso_tipo = "ACK" if random.random() < 0.85 else "NAK"
            accion_tipo = "BLCK" if random.random() < 0.5 else "DBLCK"
            
            log = iotlogs(
                iot_id=relacion.id,
                instante=instante,
                acceso=acceso_tipo,
                accion=accion_tipo
            )
            
            logs.append(log)
        
        # Ordenar logs por fecha (más antiguos primero para mantener coherencia)
        logs.sort(key=lambda x: x.instante)
        
        for log in logs:
            db.session.add(log)
        
        db.session.commit()
        print(f"{len(logs)} logs creados")

        # 5. Mostrar resumen
        print("\n=== RESUMEN DE DATOS CREADOS ===")
        print(f"Dispositivos: {Dispositivos.query.count()}")
        print(f"Usuarios: {Usuarios.query.count()}") 
        print(f"Relaciones iot_usuario: {iot_usuario.query.count()}")
        print(f"Logs iotlogs: {iotlogs.query.count()}")
        
        # Mostrar estadísticas de los logs
        logs_aceptados = iotlogs.query.filter_by(acceso="ACK").count()
        logs_denegados = iotlogs.query.filter_by(acceso="NAK").count()
        
        print(f"Logs aceptados: {logs_aceptados}")
        print(f"Logs denegados: {logs_denegados}")

if __name__ == '__main__':
    poblar_tablas()