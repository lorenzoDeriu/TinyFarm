import sys
import struct
import socket
import threading
import select
import time



DEFAULT_HOST = "127.0.0.197"
DEFAULT_PORT = 65123
END_OF_TASK = "__END___OF____TASK__"



class Instruction:
	def __init__(self):
		self.finished = False

	def shutdown(self):
		self.finished = True



def main(host=DEFAULT_HOST, port=DEFAULT_PORT):
	instruction = Instruction()

	with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
		try:
			server.bind((host, port))
			server.listen()

			input_socket = [server]

			while len(input_socket) > 0 and not (len(input_socket) == 1 and instruction.finished):
				ready,_,_ = select.select(input_socket, [], [], 10)

				if len(ready) == 0: continue
				else:
					for c in ready:
						if c is server and not instruction.finished:
							connection, address = server.accept()
							input_socket.append(connection)
						else:
							input_socket.remove(c)
							connection_handling(c, instruction)
		except OSError: 
			print("Il server non è stato avviato. Riprovare avvio con porta e indirizzo differenti")
			print("-> python3 collector.py address port")
			return
		except KeyboardInterrupt: pass
		
		server.shutdown(socket.SHUT_RDWR)
	print("Server chiuso corretamente")



def connection_handling(connection, instruction):
	with connection:
		data = recv_all(connection, 4)
		length_result = struct.unpack("!i", data[:4])[0]

		data = recv_all(connection, length_result)
		result = data[:length_result]

		data = recv_all(connection, 4)
		length_file_name = struct.unpack("!i", data[:4])[0]

		data = recv_all(connection, length_file_name)
		file_name = data[:length_file_name]

		if file_name.decode() == END_OF_TASK: 
			instruction.shutdown() # TODO trovare soluzione migliore
			return

		print("%12s \t %s" % (result.decode(), file_name.decode()))



def recv_all(conn,n):
	chunks = b''
	bytes_recd = 0
	while bytes_recd < n:
		chunk = conn.recv(min(n - bytes_recd, 1024))
		if len(chunk) == 0:
			raise RuntimeError("socket connection broken")
		chunks += chunk
		bytes_recd = bytes_recd + len(chunk)
	return chunks



if len(sys.argv) == 1:
	main()
elif len(sys.argv) == 2:
	main(sys.argv[1])
elif len(sys.argv) == 3:
	main(sys.argv[1], int(sys.argv[2]))
else:
	print(f"usage {sys.argv[0]} [host] [port]")

