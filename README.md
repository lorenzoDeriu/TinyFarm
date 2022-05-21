# Tiny Farm
Progetto finale Laboratorio II 2021-2022

## Scelte Progettuali:

### Invio dei dati al server

I dati da inviare al server sono:
- il nome del file
- la somma dei numeri all’interno del file

Per inviare il risultato tramite socket dobbiamo convertire il risultato in formato network, tramite l'utilizzo della funzione <code>htonl</code>.

La funzione <code>htonl</code> presenta però alcune limitazioni, infatti l'argomento che viene preso deve essere nel formato <code>uint32_t</code>, ovvero un intero senza segno da massimo 32 bit.

Il risultato della somma degli elementi di un file però, potrebbe essere negativo o potrebbe non essere rappresentabile in 32 bit, quindi, al momento dell'invio al server verrà prima trasformato in stringa con l'utilizzo di <code>sprintf()</code>

```C
if ((result_length = sprintf(result_str, "%ld", result)) < 0) xtermina("sprintf error", INFO);
```

dopodiché verrà inviato al server il numero di caratteri della stringa, <code>result_lenght</code>, attraverso la funzione <code>send_to()</code> 
```C
void send_to(int socket_file_descriptor, void *data, size_t data_size) {
	int writen_return_value = writen(socket_file_descriptor, data, data_size);
	if (writen_return_value != data_size) {
		xtermina("write error", INFO);
	}
}
```
che prende come argomento il file descriptor del socket, un puntatore al dato da inviare che viene castato a void per garantire la genericità della funzione e il numero di byte da inviare.

Subito dopo aver inviato la dimensione della stringa viene inviata la stringa contenente il risultato, <code>result_str</code>.
Il nome del file viene inviato con lo stesso meccanismo.



