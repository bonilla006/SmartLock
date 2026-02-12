# Proposito
SmartLock es una simulacion de una aplicación web desarrollada con Flask que permite gestionar cerraduras inteligentes mediante comunicación MQTT. La aplicación proporciona una interfaz para que los usuarios puedan controlar remotamente sus dispositivos IoT de seguridad, monitorear accesos y gestionar múltiples cerraduras.
- El mismo cuenta con dos componentes:
    - La aplicacion Flask (servidor y mobile app)
    - el dispositivo iot atraves de wokwi

# Ejecucion
- active el ambiente virutal: source api/bin/activate
    - en el caso de que no tenga un ambiente virtual: python -m venv api
- luego dentro del ambiente virtual: pip install -r requirements.txt
- corra la aplicacion flask: flask --app app.py --debug run
- para ver data defaulf:
    - poble la bd: python poblar_db.py
- corra la simulacion del iot en wokwi
    - entre al siguiente enlace en la web: https://wokwi.com/projects/448097982230388737
    - debe darle al boton verde en la esquina donde esta el dispositivo simulado
    - en el caso de que no se conecte al internet proporcionado por wokwi vulva a darle al boton

# Endpoints
- Crear_Cuenta
    - Se realiza una consulta de existencia a la tabla Usuarios
    - El valor de la misma se guarda en una variable bool
    - Se niega dicho valor
        - esto se hace ya que es mas facil trabajar con lo contrario
        - si la consulta resulto falsa es porque no hay un usuario registrado con el email ingresado 
    - Si entra a el bloque if
        - se crea la instancia Usuario y se guarda
    - se redirige al respectivo endpoint

- Inicio
    - Se realiza una consulta de existencia a la tabla Usuarios
    - El valor de la misma se guarda en una variable bool
    - Si entra a el bloque if
        - se realiza una consulta para obtener el record
        - se compara el hash de la contraseña con la contraseña en texto plano ingresada
            - con el uso de la libreria check_password_hash se hash el texto plano
        - si estas son iguales permite el acceso

- Panel_Control
    - Primero se verifica que el usuario este autenticado con el decorador @login_required
    - Luego se hace una consulta a la tabla iot_usuario usando current_user.id
        - esta es una funcion que provee flask para conseguir el id del usuario usando la aplicacion
    - Los datos de la consulta se guardan en el objeto info_IoT
    - Se crea un objeto csrf personal
        - esto es para poder interctuar con la accion de Desbloquear/Bloquear
        - debido a que en la plantilla html envuelo los botones en formularios POST
    - Se crea el objeto context
        - esta es la lista donde guardo toda la informacion de los dispositivos empaquetada
    - Se crea el objeto infodict
        - este es el diccionario donde empaqueto toda la informacion del dispositivo
    - Se verifica si en info_IoT hay datos
        - en el caso que no, la plantilla maneja el caso
    - Al entrar al If se itera por info_IoT
        - por cada dispositivo del usuario hago una consulta a la tabla iotlogs
            - limitado por los 5 mas recientes
        - los datos de esa consulta se guardan en el objeto logs
        - luego formateo los logs para que el usuario los pueda ver de una mejor manera
        - estos logs se guardan en el diccionario infologs
        - por ultimo construyo el objeto infodict

- All_Logs
    - Primero se verifica que el usuario este autenticado con el decorador @login_required
    - Se recibe del endpoint Panel_Control el id llamado rel_id
        - este es el id del record iot_usuario
    - Se hace una consulta a la tabla iot_usuario con el rel_id y con current_user.id
    - Estos datos se guardan en el objeto iot
    - Se hace una consulta a la tabla iotlogs utilizando el id consguido del objeto iot
        - esto se hace para asegurarnos que el usuario que esta accediendo es el propietario de la relacion
        - con esto aplicamos el principio de mediación completa
        - ya que no nos confiamos en el id que estamos recibiendo
    - Se crea el objeto context 
        - esta es la lista donde se guarda todos los logs
    - Se verifica si en logs hay datos
    - Al entrar al If se itera por logs
        - por cada log se formatea la informacion
        - se guarda en context

- Bloquear_Dispositivo
    - Primero se verifica que el usuario este autenticado con el decorador @login_required
    - Se recibe del endpoint Panel_Control el id llamado rel_id
        - este es el id del record iot_usuario
    - Se hace una consulta a la tabla iot_usuario con el rel_id y con current_user.id
        - esto se hace para asegurarnos que el usuario que esta accediendo es el propietario de la relacion
        - con esto aplicamos el principio de mediación completa
        - ya que no nos confiamos en el id que estamos recibiendo
    - Se crea un objeto para publicar el mensaje de bloqueo hacia el sistema de manejo mqtt en el dispositivo
    - Se cambia el estado a bloqueado
    - Se crea una instancia logs en iotlogs
    
- Desbloquear_Dispositivo
    - Primero se verifica que el usuario este autenticado con el decorador @login_required
    Se recibe del endpoint Panel_Control el id llamado rel_id
        - este es el id del record iot_usuario
    - Se hace una consulta a la tabla iot_usuario con el rel_id y con current_user.id
        - esto se hace para asegurarnos que el usuario que esta accediendo es el propietario de la relacion
        - con esto aplicamos el principio de mediación completa
        - ya que no nos confiamos en el id que estamos recibiendo
    - Se crea un objeto para publicar el mensaje de desbloqueo hacia el sistema de manejo mqtt en el dispositivo
    - Se cambia el estado a desbloqueado
    - Se crea una instancia logs en iotlogs

- Nuevo_Dispositivo
    - Primero se verifica que el usuario este autenticado con el decorador @login_required
    - Se verifica autenticación
    - Se crea una instancia del formulario NuevoDispositivo
    - Si el método es POST y el formulario es válido
    - Se obtiene el usuario actual con current_user.id
    - Se busca el dispositivo con una consulta a Dispositivos
    - Se verifica si ya existe una relación entre el usuario y el dispositivo
    - Si no existe relación:
        - Se crea nueva instancia de iot_usuario

# Manejadores
- manejador_conexion
    - Si rc == 0
        - Se suscribe a tres tópicos:
        - smartlock/+/inicio - Para inicialización de dispositivos
        - smartlock/+/comando - Para recepción de comandos
        - smartlock/+/respuesta - Para respuestas de dispositivos

- manejador_mensajes_mqtt
    - Se parsea el tópico: miapp/IOT_ID/topico
    - Se decodifica y parsea el payload JSON según el tópico:
    - Tópico: respuesta
        - Acción serv-ack: Identificación del IoT
            - Se obtiene user_id del payload
            - Se busca relación usuario-dispositivo en la base de datos
            - Si existe relación, se responde con {"acc":"iot-ack","rel_id":..., "estado":...}
        - Acción serv-wrg-pssw: Contraseña incorrecta
            - Se incrementa contador de intentos fallidos
            - Se crea log de acceso denegado
            - Si intentos ≥ 5: Bloqueo de 5 minutos
            - Si intentos < 5: Bloqueo de 30 segundos
            - Se envía comando de timeout al dispositivo
        - Acción error: Manejo de errores del dispositivo
    - Tópico: comando
        - Acción serv-val-pssw: Validación de contraseña
            - Se obtiene hash de contraseña del dispositivo
            - Se compara con hash almacenado en base de datos
            - Si coinciden:
            - Se resetean intentos fallidos (si existen)
            - Se responde con validación exitosa
            - Si no coinciden:
            - Se responde con validación fallida
        - Acción serv-dsblk: Solicitud de desbloqueo desde IoT
            - Se actualiza estado del dispositivo a "Desbloqueado"
            - Se crea log de desbloqueo exitoso
            - Se envía confirmación al dispositivo

# Referencias
- https://upkeep.com/learning/how-iot-works/
- https://www.leverege.com/iot-ebook/how-iot-systems-work
- https://www.leverege.com/iot-ebook/iot-gateways
- https://www.leverege.com/iot-ebook/iot-cloud-explained
- https://stackoverflow.com/questions/32091826/how-iot-devices-connect-to-servers
- http://www.steves-internet-guide.com/mqtt-python-beginners-course/
- https://wtforms.readthedocs.io/en/3.0.x/
- https://stackoverflow.com/questions/22084886/add-a-css-class-to-a-field-in-wtform
- https://stackoverflow.com/questions/6262943/sqlalchemy-how-to-make-django-choices-using-sqlalchemy
- https://stackoverflow.com/questions/10059345/sqlalchemy-unique-across-multiple-columns
- https://www.emqx.com/en/blog/how-to-use-mqtt-in-flask
- https://docs.python.org/3/library/hashlib.html#hashlib.sha256
- https://stackoverflow.com/questions/49032927/how-to-exit-mqtt-forever-loop
- https://iot.stackexchange.com/questions/5897/publish-mqtt-message-via-http-get
- https://codepen.io/raubaca/pen/PZzpVe
- https://stackoverflow.com/questions/13855111/how-can-i-convert-24-hour-time-to-12-hour-time
- https://www.youtube.com/watch?v=ZLt_BqmmKKE&t=5s, DoorLock system
- https://docs.wokwi.com/parts/wokwi-arduino-uno
- https://docs.wokwi.com/parts/wokwi-membrane-keypad
- https://arduinogetstarted.com/reference/library/servo-write
- https://arduinogetstarted.com/reference/library/lcd-setcursor
- https://docs.oyoclass.com/unoeditor/Libraries/keypad/
- https://codebender.cc/library/Keypad#utility%2FKey.h, def de NO_KEY 
- https://playground.arduino.cc/Code/Password/
- https://www.youtube.com/watch?v=lEXQ9w1z7Aw, simulacion IoT en wokwi
- https://iotbyhvm.ooo/how-to-simulate-an-iot-project-on-wokwi-with-mqtt-and-esp32/
- http://pubsubclient.knolleary.net/api
- http://pubsubclient.knolleary.net/api#setCallback
- http://www.steves-internet-guide.com/using-arduino-pubsub-mqtt-client/
- https://hackaday.io/project/159183-esp32-arduino-tutorial-mbed-tls-using-the-sha-256/details
- https://mbed-tls.readthedocs.io/projects/api/en/development/api/file/md_8h/#_CPPv417mbedtls_md_updateP20mbedtls_md_context_tPKh6size_t
- https://arduinojson.org/v7/tutorial/deserialization/


