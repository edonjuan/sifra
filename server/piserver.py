import time, serial, json
import mysql.connector

'''
1. Conexión a la BD.
2. Lectura de ID.
3. Mostrar ese ID.
4. Lectura de matricula.
5. Consulta existencia matricula.
6. Si exite, notificar.
7. Si no existe, registrar y notificar.

        #Mostrar las bases de datos
        cursor.execute("SHOW databases")
        row=cursor.fetchall()
        print("DB {}".format(row))
        
        #obtener registros de una tabla
        query = ("Select * From usuarios")
        cursor.execute(query)
        for x in cursor:
            print(x)
'''

def puertos_seriales():
    ports = ['/dev/ttyUSB%s' % (i + 1) for i in range(-1,20,1)]
    encontrados = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            encontrados.append(port)
        except (OSError, serial.SerialException):
            pass
    return encontrados

def register():
    uid = msg["uid"]
    matricula = int(input("Ingrese su matricula: "))
    mac = msg["mac"]
    #Se debe verificar la existencia o no del UID
    response=query(type=1,uid=uid)
    if response is False:
        response=query(type=2,uid=uid,matricula=matricula)
        if response is False:
            query(type=3,uid=uid,matricula=matricula)
        elif response is True:
            print('''

ESTA MATRICULA YA ESTA REGISTRADA

''')
            respuesta=mac+",9\n"
            respuesta=respuesta.encode('utf-8')
            puertoSerial.write(b''+respuesta)
    elif response is True:
        print('''

ESTA TARJETA YA SE ENCUENTRA REGISTRADA

''')
        respuesta=mac+",8\n"
        respuesta=respuesta.encode('utf-8')
        puertoSerial.write(b''+respuesta)

def query(**kwargs):
    #TIPO 1 es par verificar un UID antes de realizar un registro.
    if(kwargs["type"]==1):
        try:
            uid = kwargs["uid"]
            query=(
                "Select * from alumno where uid = %s"
            )
            data = (uid,)
            cursor.execute(query,data)
            #Retorno una lista de tuplas.
            #print(cursor.fetchall())
            #Retorna los warningns.
            #print(cursor.fetchwarnings())
            response = cursor.fetchall()
            if (len(response)==0):
                return False
            elif (len(response)!=0):
                return True
        except Exception as e:
            print('ERROR TIPO1: ',e)
    elif(kwargs["type"]==2):
    #TIPO 2 es para verificar una matricula antes de reaizar un registro.
        try:
            erase = cursor.fetchall()
            matricula = kwargs["matricula"]
            query=(
                "Select * from alumno where matricula=%s"
            )
            data = (matricula, )
            cursor.execute(query,data)
            response = cursor.fetchall()
            if (len(response)==0):
                return False
            elif (len(response)!=0):
                return True
        except Exception as e:
            print('ERROR TIPO2: ',e)
        pass
    elif(kwargs["type"]==3):
    #TIPO 3 es para realizar un registro.
        try:
            erase = cursor.fetchall()
            uid = kwargs["uid"]
            matricula = kwargs["matricula"]
            query = (
                "INSERT INTO alumno"
                "(uid,matricula,estatus)"
                "VALUES(%s,%s,%s);"
            )
            data = (uid,matricula,1)
            cursor.execute(query,data)
            conexion.commit()
            print("Registro exitoso")
        except Exception as e:
            print('ERROR TIPO3: ',e)
    elif(kwargs["type"]==4):
    #TIPO 4 es para realizar consulta de una credencial.
        try:
            uid = kwargs["uid"]
            mac = kwargs["mac"]
            query=(
                "Select * From alumno where uid=%s and estatus=%s"
            )
            data = (uid,1)
            cursor.execute(query,data)
            response = cursor.fetchall()
            #print(len(response))
            puertoSerial.flush()
            if (len(response)==0):
                print('''

El usuario no est      registrado

''')
                msg = mac+",0\n"
                msg = msg.encode('utf-8')
                puertoSerial.write(b''+msg)
            elif (len(response)!=0):
                msg = mac+",1\n"
                msg = msg.encode('utf-8')
                puertoSerial.write(b''+msg)
                query=("Select tipo_acceso from lugar")
                cursor.execute(query)
                acceso = cursor.fetchone()
                #print(acceso[0])
                query=("Select id_lugar from lugar where mac=%s")
                data = (mac,)
                borrar = cursor.fetchall()
                cursor.execute(query,data)
                id_lugar = cursor.fetchone()
                #print(id_lugar)
                query=("Select id_alumno from alumno where uid=%s")
                data = (uid,)
                cursor.execute(query,data)
                id_usuario = cursor.fetchone()
                #print(id_usuario)
                if (acceso[0] == 1):
                    #print("ESTOY EN EL IF DE BITACORAS")
                    estatus_acceso = "Permitido"
                    erase = cursor.fetchall()
                    query = ("Insert into bitacoras (id_alumno,id_lugar,estatus_acceso) Values (%s,%s,%s)")
                    data = (id_usuario[0],id_lugar[0],estatus_acceso)
                    cursor.execute(query,data)
                    print("Se registro en Bitacora")
                    conexion.commit()
                elif (acceso[0] == 0):
                    estatus_acceso = "Denegado"
                    erase = cursor.fetchall()
                    query = ()
                    query = ("Insert into bitacoras (id_alumno,id_lugar,estatus_acceso) Values (%s,%s,%s)")
                    data = (id_usuario[0],id_lugar[0],estatus_acceso)
                    cursor.execute(query,data)
                    conexion.commit()
        except Exception as e:
            print('ERROR EN TIPO 4: ',e)
estado = False

while(True):
    try:
        #Intanciar puerto serial
        #Escanear los puertos activos, para asegurar que el sistema funcione cuadno se reinicie por corte de luz
        puertos = puertos_seriales()
        #Obtener el puerto activo
        puerto = puertos[0]
        puertoSerial = serial.Serial(puerto, 9600)
        #Reliazar la conexi      n con la BD.
        conexion = mysql.connector.connect(host='127.0.0.1', port=3306,user='root', passwd='admin#2022', db="rfid")
        cursor = conexion.cursor()
    except KeyboardInterrupt:
        break
    except Exception as e:
        print(e)
    
    try:
        if conexion.is_connected() and estado == False:
            print("CONEXI   ^sN EXITOSA")
            #Mostrar la informaci      n del servidor
            info_server = conexion.get_server_info()
            print(info_server)
            cursor.execute("SELECT DATABASE()")
            row=cursor.fetchone()
            print("Conectado a la base de datos: {}".format(row))
            estado = True
        elif conexion.is_connected() == False:
            print("FALLO LA CONEXI   ^sN")
            estado = False

        if estado == True:
            print("PASE SU TARJETA POR EL LECTOR")
            #Leer el json del puerto serial
            msg = json.loads(puertoSerial.readline())
            #imprimir el json que se recibe
            #print(msg)
            if msg["type"]=="request":
                register()
            elif msg["type"]=="query":
                uid = msg["uid"]
                mac = msg["mac"]
                query(type=4,uid=uid,mac=mac)
    except KeyboardInterrupt:
        cursor.close()
        conexion.close()
        break
    except Exception as e:
        print(e)

