import paho.mqtt.client as mqtt
import time
import datetime
import MySQLdb

########## MySQL Setup ##########
# Open database connection
db = MySQLdb.connect('localhost','mqtt','raspberry','temperature' )

# prepare a cursor object using cursor() method
cursor = db.cursor()

########## MQTT Setup ##########
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("sensor0")
    client.subscribe("sensor1")
    client.subscribe("sensor2")
    client.subscribe("sensor3")

client = mqtt.Client()
client.on_connect = on_connect

client.connect("192.168.0.200", 1883, 60)

# Start looping the MQTT network interface in the background
client.loop_start()

########## Main Code ##########

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):

    #get the name of this topic, this tells us which sensor we are dealing with
    csensor = msg.topic
    #get the payload, this is the temperature the sensor is reporting minus 2 as the sensors report a little high
    ctemp = float(msg.payload)-2
    #get the zone by taking the last character of the message topic. (topics will be "sensor0", "sensor1", etc
    zone = list(csensor)[6]
    print(csensor+" %f" % ctemp)

    # Prepare SQL query to INSERT a record into the database.
    sql ="INSERT INTO temps(date, time, zone, temperature) VALUES (CURRENT_DATE(), NOW(), '%s', '%f')" % (csensor, ctemp)
    try:
       # Execute the SQL command
       cursor.execute(sql)
       # Commit changes to database
       db.commit()
       print("success")
    except MySQLdb.Error, e:
        try:
            print "MySQL Error [%d]: %s" % (e.args[0], e.args[1])
        except IndexError:
            print "MySQL Error: %s" % str(e)
        # Rollback if  error
        db.rollback()
    #switch on heater if temp in zone is less then 20
    if ctemp <= 24:
        client.publish("HEATER_FEED", zone+"1", 1)
    else:
        client.publish("HEATER_FEED", zone+"0", 1)


client.on_message = on_message


# Main loop
while True:
    # send temperature request
    client.publish("HEATER_FEED", "99", 1)
    # wait for 15 minutes
    time.sleep(15*60)
