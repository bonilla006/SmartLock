from dotenv import load_dotenv
import os
load_dotenv()

basedir = os.path.abspath(os.path.dirname(__file__))

if bool(os.getenv('DEVENV')): 
    database = f'sqlite:///{os.path.join(basedir, "app.db")}'
else:
    database = os.getenv('DATABASE')
    
class Config:
    #Configuraciones para la Base de datos
    SQLALCHEMY_DATABASE_URI = database
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    SECRET_KEY = os.getenv('SECRET_KEY')

    #Configuraciones para el MQTT Broker
    MQTT_BROKER_URL = os.getenv('MQTT_BROKER_URL')
    MQTT_BROKER_PORT = int(os.getenv('MQTT_BROKER_PORT'))
    MQTT_USERNAME = os.getenv('MQTT_USERNAME')
    MQTT_PASSWORD = os.getenv('MQTT_PASSWORD')
    MQTT_KEEPALIVE = int(os.getenv('MQTT_KEEPALIVE'))
    MQTT_TLS_ENABLED = False
