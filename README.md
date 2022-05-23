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
		xtermina("writen error", INFO);
	}
}
```
che prende come argomento il file descriptor del socket, un puntatore al dato da inviare che viene castato a void per garantire la genericità della funzione e il numero di byte da inviare.

Subito dopo aver inviato la dimensione della stringa viene inviata la stringa contenente il risultato, <code>result_str</code>.
Il nome del file viene inviato con lo stesso meccanismo.

### Ricezione dei dati del client

Il server gestisce la ricezione dei dati da parte dei client tramite l'utilizzo della funzione <code>select()</code> che permette di gestire anche più richieste di connessione alla volta.
Nel caso di connessioni multiple alcuni client potrebbero dover rimanere in attesa, tuttavia, poiché il tempo di gestione della connessione è molto rapido, l'eventuale tempo di attesa dei client è trascurabile.

### Gestione del segnale SIGINT

La gestione del segnale SIGINT è stato assegnata alla funzione <code>handler</code> che si limita a cambiare la variabile booleana globale <code>_interrupt</code> che viene controllata all'interno della guardia del ciclo for in cui vengono inviati i nomi dei file ai thread worker

```C
for (int i = optind; i < argc && !_interrupt; i++)
```

Al momento in cui la variabile <code>_interrupt</code> viene settata a _true_ il ciclo si interrompe fermando l'invio dei nomi dei file, dopodiché manda i segnali di terminazioni a tutti i thread e aspetta la loro terminazione.



