import serial
from strip_ansi import strip_ansi
import json
import time
import serial.tools.list_ports as st
import pandas as pd


def main():
    file = open("raw_data.txt", 'w+')
    while time.process_time() < 1:
        data_line = serial_readline()
        if data_line is not None:
            file.writelines(str(list(data_line.values()))[1:-1])
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
        dataline = dataline[dataline.find("{"):]
        dataline_json = json.loads(dataline)

        return dataline_json
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