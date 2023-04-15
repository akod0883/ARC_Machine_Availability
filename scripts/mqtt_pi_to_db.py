# Python3 script to connect to MQTT, read values and write them into MySQL

import paho.mqtt.client as mqtt
import mysql.connector
import smtplib
from email.message import EmailMessage

EMAIL_ADDRESS = "akhilkodumuri9@gmail.com"
EMAIL_PASSWORD = "urckvwydorlesqdg"
email_msg = EmailMessage()
email_msg['Subject'] = "Go Workout!"
email_msg['From'] = EMAIL_ADDRESS

db = mysql.connector.connect(
    host="localhost",
    database="test",
    user="root",
    passwd="password"
    )

broker_address= "172.20.10.7"          #Broker address
port = 1883                          #Broker port
# user = "***MQTT_USERNAME***"         #Connection username
# password = "***MQTT_PASSWORD***"     #Connection password

def send_email():
    pass

def on_connect(client, userdata, flags, rc):  # The callback for when the client connects to the broker
    print("Connected with result code {0}".format(str(rc)))  # Print result of connection attempt
    client.subscribe("esp32/machine_availability")

def on_message(client, userdata, msg):  # The callback for when a PUBLISH message is received from the server.
    id_, avail = msg.payload.decode('utf-8').split()
    print("AVAIL: ", avail)
    print("ID: ", id_)

    cur = db.cursor()

    sql_insert = "UPDATE availability SET availability = {} WHERE unitID = {};".format(avail, id_)
    cur.execute(sql_insert)
    db.commit()
    print("data received and imported in MySQL")

    # if machine becomes available, send email to users that have subscribed
    if avail == "0":
        email_msg.set_content("Rack #{} is available for use! Go workout!".format(id_))
        rack = "rack_" + id_
        sql_insert = "SELECT email_address FROM rack_subscriptions WHERE {} = {}".format(rack, 1)
        row = cur.execute(sql_insert)
        emails = cur.fetchall()

        print("EMAILS TO SEND: ", emails)
        
        with smtplib.SMTP_SSL('smtp.gmail.com', 465) as smtp:
            smtp.login(EMAIL_ADDRESS, EMAIL_PASSWORD)
            for email in emails:
                print(email[0], type(email[0]))
                email_msg['To'] = email[0]
                smtp.send_message(email_msg)
                print("EMAIL SENT TO: ", email[0])
                del email_msg['To']
        

        
   
client = mqtt.Client("client")
client.on_connect = on_connect  # Define callback function for successful connection
client.on_message = on_message  # Define callback function for receipt of a message
client.connect(broker_address, port=port)          #connect to broker
client.loop_forever()  # Start networking daemon