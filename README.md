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

Subito dopo aver inviato il numero di caratteri, viene inviata la stringa contenente il risultato, <code>result_str</code>.
Con lo stesso meccanismo, viene inviato il nome del file.

### Ricezione dei dati del client

Il server gestisce la ricezione dei dati da parte dei client tramite l'utilizzo della funzione <code>select()</code>, che permette di gestire anche più richieste di connessione alla volta.
Nel caso di connessioni multiple, alcuni client potrebbero necessariamente dover attendere, tuttavia poiché il tempo di gestione della connessione è molto rapido, l'eventuale tempo di attesa dei client è trascurabile.

### Gestione del segnale SIGINT

La gestione del segnale SIGINT è stata assegnata alla funzione <code>handler</code>, la quale si limita a cambiare la variabile booleana globale <code>_interrupt</code>, che viene controllata all'interno della guardia del ciclo for, in cui vengono inviati i nomi dei file ai thread worker.

```C
for (int i = optind; i < argc && !_interrupt; i++)
```

Nel momento in cui la variabile <code>_interrupt</code> viene settata a _true_, il ciclo si interrompe, arrestando l'invio dei nomi dei file; successivamente manda i segnali di terminazione a tutti i thread e aspetta che la loro esecuzione sia conclusa. Infine, dopo aver deallocato la memoria dedicata al buffer, invia il segnale di terminazione al server.
