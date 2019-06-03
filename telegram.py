import sqlite3
import telepot   # Importing the telepot library
from telepot.loop import MessageLoop    # Library function to communicate with telegram bot
from re import findall
import serial
from time import strftime, sleep
from ast import literal_eval

#Location
location = 'Tank1'

#Serial port
serial_port = 'COM3'
#Connect to Serial Port for communication
ser = serial.Serial(serial_port, 9600, timeout=0.1)

#Setup a loop to send data at fixed intervals in seconds
fixed_interval = 6

#Connect to database
conn = sqlite3.connect('atlas.db')
#Create cursor for sqlite3 connection
db = conn.cursor()

def handle(msg):
    chat_id = msg['chat']['id'] # Receiving the message from telegram
    command = msg['text']   # Getting text from the message

    print ('Received:')
    print(command)

    # Comparing the incoming message to send a reply according to it
    if command == '/getlastdata':
        #Connect to database
        conn1 = sqlite3.connect('atlas.db')
        #Create cursor for sqlite3 connection
        db1 = conn1.cursor()

        
        s = db1.execute("SELECT id, ph, do, ec, rtd, date, time FROM data ORDER BY id DESC LIMIT 1")
        conn1.commit()

        row = s.fetchone();

        conductivity = findall(r"[-+]?\d*\.\d+|\d+", row[3])
        
        ec_value = float(conductivity[0])
        tds_value = int(conductivity[1], base=10)
        psu_value = float(conductivity[2])
        ssg_value = float(conductivity[3])

        msg_data = {"id":row[0], "date":row[5], "time":row[6], "type": "Atlas_Sensor", "data":{"ph": row[1], "do": row[2], "ec": ec_value, "tds": tds_value, "psu": psu_value, "ssg": ssg_value, "rtd": row[4]}}
        bot.sendMessage (chat_id, str(msg_data))
        
# Telegram token
bot = telepot.Bot('788813302:AAEd6UEEXLkZrckEQIchpxpfokAG5a-8118')
print (bot.getMe())

# Start listening to the telegram bot and whenever a message is  received, the handle function will be called.
MessageLoop(bot, handle).run_as_thread()
print ('Listening....')

while 1:
    try:
        #Value obtained from Arduino + Atlas sensors
        predata = ser.readline()
        predata_utf = predata.decode("utf-8")
        data = literal_eval(predata_utf)

        #Extract data from serial port
        EC = data['EC']
        pH = data['pH']
        DO = data['DO']
        RTD = data['RTD']

        #current time and date
        time_hhmmss = strftime('%H:%M:%S')
        date_mmddyyyy = strftime('%d/%m/%Y')

        #current location name
        print(f"{time_hhmmss}, {date_mmddyyyy}, {location}")

        print(f"EC: {EC} --- pH: {pH} --- DO: {DO} --- RTD: {RTD}")

        #Insert a row of data to data table
        db.execute("INSERT INTO data(location, ec, ph, do, rtd, date, time) VALUES (?,?,?,?,?,?,?)",(location, EC, pH, DO, RTD, date_mmddyyyy, time_hhmmss))
        conn.commit()

        sleep(fixed_interval)

    except IOError:
        print('Something has been wrong!')
        sleep(fixed_interval)
    except SyntaxError:
        print('Something has been wrong!')
        sleep(fixed_interval)
    except IndexError:
        print('Something has been wrong!')
        sleep(fixed_interval)
    except:
        print('Something has been wrong!')
        sleep(fixed_interval)
