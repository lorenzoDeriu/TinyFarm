# Tiny Farm
Final project Laboratory II 2021-2022

## Design choices:

### Sending data to the server

The data to be sent to the server are:
- the file name;
- the sum of the numbers inside the file.

To send the result through the socket, we must convert the result to network format, using the <code>htonl</code> function.

However, the <code>htonl</code> function has some limitations, as the argument taken must be in the <code>uint32_t</code> format, that is a 32-bit unsigned integer.

The result of the sum of the elements of a file may be negative, or may not be representable in 32 bits, so when sending to the server it is necessary to first transform it into a string, using the <code>sprintf()</code> function.

```C
if ((result_length = sprintf(result_str, "%ld", result)) < 0) xtermina("sprintf error", INFO);
```

Then the information about the number of characters in the string, <code>result_lenght</code>, is sent to the server through the <code>send_to()</code> function;
```C
void send_to(int socket_file_descriptor, void *data, size_t data_size) {
	int writen_return_value = writen(socket_file_descriptor, data, data_size);
	if (writen_return_value != data_size) {
		xtermina("writen error", INFO);
	}
}
```
which takes as arguments:
- the socket file descriptor;
- a pointer to the data to be sent, which is converted into a void pointer to ensure the genericity of the function;
- the number of bytes to be sent.

Immediately after sending the number of characters, the string containing the result, <code>result_str</code>, is sent.
In the same way, the name of the file is sent.

### Receiving client data

The server manages the reception of data from clients using the <code>select()</code> function, which also allows multiple connection requests to be handled at the same time.
In the case of multiple connections, some clients may necessarily have to wait, however, since the connection management time is very short, the waiting time of clients is negligible.

### Handling the SIGINT signal

The handling of the SIGINT signal was assigned to the <code>handler</code> function, which simply changes the global boolean variable <code>_interrupt</code>, which is checked within the guard of the for loop, in which the names of the files are sent to the worker threads.


```C
for (int i = optind; i < argc && !_interrupt; i++)
```

When the <code>_interrupt</code> variable is set to true, the loop is interrupted, stopping the sending of the file names; then it sends the SIGINT signals to the worker threads, which will also stop their activity.
