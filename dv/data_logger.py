import serial
from strip_ansi import strip_ansi
import json
import time
import serial.tools.list_ports as st
import pandas as pd


def main():
    file = open("data/walking_fast_3.txt", 'w+')
    start = time.time()
    while time.time() - start < 30:
        # print(time.process_time())
        data_line = serial_readline()
        if data_line is not None and len(data_line) > 5:
            # print(data_line)
            file.write(f"{data_line}\n")
    file.close()


def serial_readline():
    global ser
    try:
        ser.flushInput()
        dataline = ser.readline().decode().strip().replace(" ", "")
    except Exception:
        print("Reconnecting...")
        ser = init_serial()
    try:
        dataline = strip_ansi(dataline)
        dataline = dataline.replace("CSSE4011:~$", "")

        dataline = dataline[dataline.find("Datais"):]
        dataline = dataline.replace("Datais","")
        # dataline_json = json.loads(dataline)

        return dataline
    except Exception:
        print("Failed to parse JSON")
        return

def init_serial():
    # ======== CONNECT ==========
    try:
        ports = st.comports()
        name = ""

        for port in ports:
            print(f"device:{port.device} - name:{port.name} - description:{port.description} - hwid:{port.hwid}")
            hwid = port.hwid[12:16]
            if hwid == "C553":
                name = port.name
                print("selected " + name)
                break

        # name = "/dev/" + name
        ser = serial.Serial(name, 115200)
        return ser

    except Exception:
        time.sleep(1)
        init_serial()

main()