from flask import Flask, render_template, request, redirect, url_for, jsonify
from flask_login import LoginManager, login_user, logout_user, login_required, current_user
from flask_wtf import FlaskForm, CSRFProtect
from flask_wtf.csrf import generate_csrf
from flask_mqtt import Mqtt
from wtforms import StringField, PasswordField
from wtforms.validators import InputRequired, Length, Email, ValidationError
from werkzeug.security import generate_password_hash, check_password_hash
from config import Config
from models import *
from datetime import datetime
from hashlib import sha256
import phonenumbers
import json
import re


###########################################################################################################
############################################CORE###########################################################
###########################################################################################################
app = Flask(__name__)
#cargar configuraciones
app.config.from_object(Config)
# Inicializar y crear la base de datos
crear_bd(app)
#para autenticacion con werkzeug
login_manager = LoginManager(app)
#para formularios
csrf = CSRFProtect(app)
#MQTT
asunto = "/smartlock/mqtt/"
IOT_ID = None
cliente_mqtt = Mqtt(app)
# Verificar configuración MQTT cargada
print("=== CONFIGURACIÓN MQTT ===")
print(f"MQTT_BROKER_URL: {app.config.get('MQTT_BROKER_URL')}")
print(f"MQTT_BROKER_PORT: {app.config.get('MQTT_BROKER_PORT')}")
print(f"MQTT_KEEPALIVE: {app.config.get('MQTT_KEEPALIVE')}")
print(f"MQTT connected: {cliente_mqtt.connected}")
###########################################################################################################
############################################CORE###########################################################
###########################################################################################################

###########################################################################################################
############################################FORMULARIOS####################################################
###########################################################################################################
#Inicio de seccion
class ValidarUsuario(FlaskForm):
    email = StringField(label="Email", render_kw={"placeholder":"Ingrese su correo electronico"},validators=[Email(), InputRequired(), Length(10, 50)])
    passw = PasswordField(label="Contraseña", render_kw={"placeholder":"Ingrese su contraseña"})

class CrearUsuario(FlaskForm):
    email = StringField(label="Email", validators=[Email(), InputRequired(), Length(10, 50)])
    nombre = StringField(label="Nombre")
    apellido = StringField(label="Apellido")
    telefono = StringField(label="Telefono")
    passw = PasswordField(label="Contraseña")
    
    def validate_telefono(self, telefono):
        try:
            tel = phonenumbers.parse(telefono.data)
            if not phonenumbers.is_valid_number(tel):
                raise ValueError()
        except (phonenumbers.phonenumberutil.NumberParseException, ValueError):
            raise ValidationError('Numero de telefono invalido')

    def validate_passw(self, passw):
        instancia_pssw = passw.data

        if len(instancia_pssw) < 8:
            raise ValidationError("Mínimo 8 caracteres")
        
        if len(instancia_pssw) > 129:
            raise ValidationError("Maximo 128 caracteres")
    
        if not re.search(r'[A-Z]', instancia_pssw):
            raise ValidationError("Debe tener al menos una mayúscula")
        
        if not re.search(r'[a-z]', instancia_pssw):
            raise ValidationError("Debe tener al menos una minúscula")
        
        if not re.search(r'\d', instancia_pssw):
            raise ValidationError("Debe tener al menos un número")
        
        if not re.search(r'[@$!%*?&]', instancia_pssw):
            raise ValidationError("Debe tener al menos un carácter(@$!%*?&)")

                  
class NuevoDispositivo(FlaskForm):
    id_modelo = StringField(label="Modelo")
    codigo = StringField(label="Codigo_Master")
    nombre = StringField(label="Nombre")

    def validate_id_modelo(self, id_modelo):
        #verificar que tenga -
        mitad = len(id_modelo.data)//2
        if id_modelo.data[mitad] != "-":
            raise ValidationError('El formato del id es: AAAA-1234')
        
        #separa el texto y lo numerico
        texto, numero = id_modelo.data.split("-")

        if not texto.isupper():
            raise ValidationError('Deben ser mayusculas')
        
        if len(texto) != 4 or len(numero) != 4:
            raise ValidationError('El id debe ser de 8 carcateres')
    
        if not texto.isalpha():
            raise ValidationError('los primeros 4 caracteres deben ser letras')
        
        if not numero.isdigit():
            raise ValidationError('los ultimos 4 caracteres deben ser numeros')
    
    def validate_codigo(self, codigo):
        if not codigo.data.isdigit():
            raise ValidationError('El codigo debe ser numerico')
        
        if len(codigo.data) < 4:
            raise ValidationError('Minimo 5 digitos')
###########################################################################################################
############################################FORMULARIOS####################################################
###########################################################################################################

###########################################################################################################
############################################ENDPOINTS######################################################
###########################################################################################################
@login_manager.user_loader
def load_user(id_usuario):
    return Usuarios.query.get(id_usuario)

@app.route('/crear-cuenta/', methods=['GET', 'POST'])
def Crear_Cuenta():
    form = CrearUsuario()

    if request.method == "POST":
        if form.validate_on_submit():
            exists = db.session.query(db.exists().where(Usuarios.email==form.email.data)).scalar()
            if not exists:
                nuevo_usuario = Usuarios(
                    email=form.email.data,
                    nombre=form.nombre.data,
                    apellido=form.apellido.data,
                    telefono=form.telefono.data,
                    password_hash=generate_password_hash(form.passw.data)
                )
                db.session.add(nuevo_usuario)
                db.session.commit()
                return redirect(url_for('Inicio'))
            else:
                print("este email ya existe")
                return redirect(url_for('Crear_Cuenta'))
        else:
            print("form invalido")
            
    return render_template('crear_cuenta.html', form=form)

@app.route('/inicio/', methods=['GET', 'POST'])
def Inicio():
    #para request POST
    form = ValidarUsuario()
    if request.method == 'POST':
        if form.validate_on_submit():
            #verificar si el usuario existe en la BD
            #variable bool para saber
            exists = db.session.query(db.exists().where(Usuarios.email==form.email.data)).scalar()
            if exists:
                info_usuario = Usuarios.query.filter_by(email=form.email.data).first()

                # autenticar el usuario
                if check_password_hash(info_usuario.password_hash, form.passw.data):
                    login_user(info_usuario)
                    return redirect(url_for('Panel_Control'))

                else:
                    print("mala contraseña")
                    return redirect(url_for('Inicio'))
            else:
                print("Usted no tiene una cuenta")
        else:
            print("form invalido")
    #para request GET
    return render_template('inicio.html', form=form)

#Panel principal para manejar los dispositivos
@app.route('/panel-principal/', methods=['GET'])
@login_required
def Panel_Control():
    # conseguir los dispositivos del usuario
    try:
        info_IoT = iot_usuario.query.filter_by(usuario_id=current_user.id).all()
    except Exception as err:
        print("Error en Panel_Control(info_iot):", err)

    mycsrf=generate_csrf()
    context = []
    infodict = {}
    if info_IoT:
        for info in info_IoT:
            #conseguir los logs del dispositivo 
            try:
                logs = iotlogs.query.filter_by(iot_id=info.id).order_by(iotlogs.instante.desc()).limit(5).all()
            except Exception as err:
                print("Error en Panel_Control(logs):", err)
            
            #formatear los logs
            infologs = [{
                'fecha':f"{log.instante.day}/{log.instante.month}/{log.instante.year}",
                'tiempo':f"{log.instante.strftime("%I:%M %p")}",
                'acceso':log.acceso.value,
                'accion':log.accion.value
            } for log in logs]
            
            infodict = {
                'id':info.id,
                'nombre':info.alias if info.alias else info.name,
                # 'encendido':info.encendido,
                'estado':info.estado.value,
                'carga':info.bateria,
                'logs':infologs
            }
            context.append(infodict)

    # print(context)
    return render_template('panel_principal.html', context=context, csrf=mycsrf)

@app.route('/panel-principal/logs/<int:rel_id>/', methods=['POST'])
@login_required
def All_Logs(rel_id):
    if request.method == "POST":
        #conseguir los logs del dispositivo 
        try:
            iot = iot_usuario.query.filter_by(id=rel_id, usuario_id=current_user.id).one()
            logs = iotlogs.query.filter_by(iot_id=iot.id).order_by(iotlogs.instante.desc()).all()
        except Exception as err:
            print("Error en Logs:", err)
            return redirect(url_for('Panel_Control'))
        
        context = []
        if logs:
            context = [{
                'fecha':f"{log.instante.day}/{log.instante.month}/{log.instante.year}",
                'tiempo':f"{log.instante.strftime("%I:%M %p")}",
                'acceso':log.acceso.value,
                'accion':log.accion.value
            } for log in logs]

    return render_template('logs.html', context=context)

@app.route('/panel-principal/lock/<int:rel_id>/', methods=['POST'])
@login_required
def Bloquear_Dispositivo(rel_id):
    if request.method == "POST":
        #conseguir el id del record iot-usuario
        try:
            dispositivo = iot_usuario.query.filter_by(id=rel_id, usuario_id=current_user.id).one()
        except Exception as err:
            print("Error en Bloquear_Dispositivo:", err)
            return redirect(url_for('Panel_Control'))
        
        #publicar comando para bloquear dispositivo
        ret_payload = {"acc":"iot-blk", "estado":"Bloqueado"}
        cliente_mqtt.publish(f"smartlock/{dispositivo.iot.id_modelo}/comando", json.dumps(ret_payload))

        #actualizar el estado del dispositivo de desbloqueado a bloqueado
        dispositivo.estado = "Bloqueado"
        db.session.commit()

        #crear log
        nuevo_log = iotlogs(iot_id=rel_id, instante=datetime.now(), acceso="ACK", accion="BLCK")
        db.session.add(nuevo_log)
        db.session.commit()

        #volver al panel principal
        return redirect(url_for('Panel_Control'))
    else:
        print("metodo incorrecto")
        return redirect(url_for('Panel_Control'))
    
@app.route('/panel-principal/unlock/<int:rel_id>/', methods=['POST'])
@login_required
def Desbloquear_Dispositivo(rel_id):
    if request.method == "POST":
        #conseguir el id del record iot-usuario
        try:
            dispositivo = iot_usuario.query.filter_by(id=rel_id, usuario_id=current_user.id).one()
        except Exception as err:
            print("Error en Desbloquear_Dispositivo:", err)
            return redirect(url_for('Panel_Control'))
        
        #publicar comando para desbloquear el dispositivo
        ret_payload = {"acc":"iot-dsblk", "estado":"Desbloqueado"}
        cliente_mqtt.publish(f"smartlock/{dispositivo.iot.id_modelo}/comando", json.dumps(ret_payload))

        dispositivo.estado = "Desbloqueado"
        db.session.commit()
        
        #crear log
        nuevo_log = iotlogs(iot_id=rel_id, instante=datetime.now(), acceso="ACK", accion="DBLCK")
        db.session.add(nuevo_log)
        db.session.commit()

        #volver al panel principal
        return redirect(url_for('Panel_Control'))
    else:
        print("metodo incorrecto")
        return redirect(url_for('Panel_Control'))

@app.route('/panel-principal/nuevo-dispositivo/', methods=['GET', 'POST'])
@login_required
def Nuevo_Dispositivo():
    form = NuevoDispositivo()

    if request.method == "POST":
        if form.validate_on_submit():
            #conseguir al usuario y el dispositivo
            try:    
                u_id = Usuarios.query.get(current_user.id)
                d_id = Dispositivos.query.filter_by(id_modelo=form.id_modelo.data).first()
                relacion = iot_usuario.query.filter_by(usuario_id=u_id.id, iot_id=d_id.id).all()
            except Exception as err:
                print("Error al buscar data de usuario o dispositivo:", err)
                return redirect(url_for('Nuevo_Dispositivo'))
            
            if not relacion:
                nuevo_dispositivo = iot_usuario(
                    iot_id=d_id.id, 
                    usuario_id=u_id.id, 
                    codigo=sha256(form.codigo.data.encode()).hexdigest(),
                    encendido=True,
                    estado="Bloqueado", 
                    alias=form.nombre.data,
                    bateria=100
                )
                db.session.add(nuevo_dispositivo)
                db.session.commit()
                return redirect(url_for('Panel_Control'))
            else:
                print("ya tiene conectado este dispositivo")
                return redirect(url_for('Nuevo_Dispositivo'))
        else:
            print("error en Nuevo_Dispositivo:",form.errors)
            return redirect(url_for('Nuevo_Dispositivo'))
            
    return render_template('nuevo_dispositivo.html', form=form)

# @app.route('/reset', methods=['GET'])
# def reset_try():
#     if request.method == "GET":
#         rel_id =  request.args['rel_id']
#         print(rel_id)
#         try:
#             dispositivo = iot_usuario.query.get(rel_id) 
#         except Exception as err:
#             print("Error en Desbloquear_Dispositivo:", err)

#         dispositivo.intentos = 0
#         db.session.commit()
#     return "reset echo"

###########################################################################################################
############################################ENDPOINTS######################################################
###########################################################################################################

###########################################################################################################
############################################MANEJADOR MQTT#################################################
###########################################################################################################
#Se encarga de monitorear los asuntos a los que el servidor va a escuchar
@cliente_mqtt.on_connect()
def manejador_conexion(client, userdata, flags, rc):
    try:
        print(f"=== EVENTO ON_CONNECT ===")
        if rc == 0:
            print("Conexión MQTT exitosa al broker")
            # suscribirse al topico de inicio
            status_inicio = cliente_mqtt.subscribe("smartlock/+/inicio")
            # suscribirse al topico de comandos
            status_comandos = cliente_mqtt.subscribe("smartlock/+/comando")
            # suscribirse al topico de estados
            status_status = cliente_mqtt.subscribe("smartlock/+/respuesta")
            print(f"Subscripciones: [{status_inicio}], [{status_comandos}], [{status_status}]")
        else:
            print(f"Error de conexión MQTT: {rc}")
            
    except Exception as err:
        print("Error al conectarse a un topico MQTT:", err)


@cliente_mqtt.on_message()
def manejador_mensajes_mqtt(client, userdata, message):
    try:
        print(f"=== EVENTO ON_MESSAGE ===")
        # asunto_recibido = message.topic
        print(message.topic)
        miapp, IOT_ID, topico = message.topic.split("/")
        payload = message.payload.decode()
        print("raw data:", payload)
        data = json.loads(payload)
        if topico == "respuesta":
            if 'acc' in data:
                #identificacion del IoT con el servidor
                if data['acc'] == "serv-ack":
                    print("ACK")
                    #conseguir el id de usuario
                    user_id = data.get('user_id', 'N/A')
                    if user_id != 'N/A' and IOT_ID:
                        with app.app_context():
                            try:    
                                u_id = Usuarios.query.get(user_id)
                                d_id = Dispositivos.query.filter_by(id_modelo=IOT_ID).first()
                                relacion = iot_usuario.query.filter_by(usuario_id=u_id.id, iot_id=d_id.id).first()
                            except Exception as err:
                                print("Error al buscar data de usuario o dispositivo:", err)                    

                        if relacion:
                            #publicar el id de relacion
                            ret_payload = {"acc":"iot-ack","rel_id":relacion.id, "estado":relacion.estado.value}
                            cliente_mqtt.publish(f"smartlock/{IOT_ID}/respuesta", json.dumps(ret_payload))
                        else:
                            print(f"problema al buscar la relacion {IOT_ID}-{user_id}")

                elif data['acc'] == "serv-wrg-pssw":
                    with app.app_context():
                        try:
                            relacion = iot_usuario.query.get(data['rel_id'])
                            if not relacion:
                                return
                                
                            relacion.intentos += 1
                            db.session.commit()

                            nuevo_log = iotlogs(iot_id=data['rel_id'], instante=datetime.now(), acceso="NAK", accion="DBLCK")
                            db.session.add(nuevo_log)
                            db.session.commit()
                            
                            if relacion.intentos >= 5:
                                # Bloqueo largo (5 minutos)
                                ret_payload = {"acc": "iot-time-out", "try": "no", "time": 300000}
                            else:
                                # Bloqueo corto (30 segundos)
                                ret_payload = {"acc": "iot-time-out", "try": "si", "time": 30000}
                                
                            cliente_mqtt.publish(f"smartlock/{IOT_ID}/comando", json.dumps(ret_payload))
                            
                        except Exception as err:
                            print("Error en wrg-pssw:", err)
                    
                elif data['acc'] == "error":
                    print(f"se recibio {data['err']} de {IOT_ID}")
                else:
                    print("error=>accion no identificada:", data['acc'])
            else:
                pass
        elif topico == "comando":
            if 'acc' in data:
                #comando de validacion de password
                if data['acc'] == "serv-val-pssw":
                    #conseguir la contraseña desde el smartlock
                    hashpssw = data.get('pssw', 'N/A')

                    #conseguir la contraseña de la bd
                    with app.app_context():
                        try:
                            relacion = iot_usuario.query.get(data['rel_id'])
                        except Exception as err:
                            print("error al buscar la relacion en el comando val-pssw:", err)

                        #validar la contraseña con la de la bd
                        if hashpssw == relacion.codigo:
                            #verificar cuantos intentos tiene el dispositivo
                            if relacion.intentos > 0:
                                #reset los intentos
                                relacion.intentos = 0
                                db.session.commit()
                                
                            #publicar que la contraseña ingresada es la correcta
                            ret_payload = {"acc":"iot-val-pssw","val":"exito"}
                            cliente_mqtt.publish(f"smartlock/{IOT_ID}/respuesta", json.dumps(ret_payload))
                        else:
                            #hacer algo para contraseñas fallidas
                            ret_payload = {"acc":"iot-val-pssw","val":"fallo"}
                            cliente_mqtt.publish(f"smartlock/{IOT_ID}/respuesta", json.dumps(ret_payload))

                #la accion de abrir viene desde el iot
                elif data['acc'] == "serv-dsblk":
                    #conseguir la relacion
                    with app.app_context():
                        try:
                            relacion = iot_usuario.query.get(data['rel_id'])
                        except Exception as err:
                            print("error al buscar la relacion en el comando val-pssw:", err)

                        #cambiar el estado del dispositivo
                        relacion.estado = "Desbloqueado"
                        db.session.commit()
                        
                        nuevo_log = iotlogs(iot_id=data['rel_id'], instante=datetime.now(), acceso="ACK", accion="DBLCK")
                        db.session.add(nuevo_log)
                        db.session.commit()

                    #mandar mensaje para que se ejecute el mecanismo de desbloqueo
                    ret_payload = {"acc":"iot-dsblk", "estado":"Desbloqueado"}
                    cliente_mqtt.publish(f"smartlock/{IOT_ID}/respuesta", json.dumps(ret_payload))
        else:
            pass
            
    except Exception as err:
        print(f"Error en manejador_mensajes_mqtt: {err}")
        #ret_payload = f'"acc":"mssg","estado":"err","error":{err}'
        #cliente_mqtt.publish(f"smartlock/{iot_id}/respuesta", ret_payload)

# @app.route('/demo-mqtt/', methods=['POST'])
# @csrf.exempt
# def Demo_Mqtt():
#     request_data = request.get_json()
#     publish_result = cliente_mqtt.publish(request_data['topic'], request_data['msg'])
#     return jsonify({'code': publish_result[0]})


###########################################################################################################
############################################MANEJADOR MQTT#################################################
###########################################################################################################

if __name__ == "__main__":
    app.run(debug=True)