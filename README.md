# Introduzione

In questo report si vuole analizzare e implementare la procedura di
generazione di un automa caratteristico per il parsing bottom-up di tipo
LR(0). Fra i vari tipi di parsing quello LR(0) è un
tipo di parsing per cui si legge l'input da sinistra a destra e si
produce una derivazione di tipo rightmost a partire dalla grammatica
data in input.

#### Il problema:

il primo step del parsing bottom-up è quello di generare un automa
caratteristico a partire dalla grammatica letta in input per poi andare
a formare una tabella di parsing. In questo report ci focalizzeremo
solamente sulla parte di generazione dell'automa caratteristico.

# Definizioni

L'automa caratteristico è formato da un insieme di stati interconnessi
da una funzione di transizione $$\tau$$ definita su coppie di stati. Ogni
stato contiene degli LR(0)-items : alcuni di questi item faranno parte
del kernel, mentre altri fanno parte della closure del kernel.\
La tecnica di costruzione dell'automa caratteristico è incrementale:
andiamo a popolare un set di stati definendo mano a mano la funzione di
transizione, fino a saturazione. Nel dettaglio l'algoritmo di
generazione dell'automa seguirà i seguenti step:
* Lettura della grammatica fornita in input dall'utente ed estrazione
    delle produzioni con rimozione di eventuali spazi dalla produzione;
* Aggiunta di una fresh production alla grammatica facendo attenzione
    ad aggiungere un simbolo nuovo in modo da evitare conflitti;
* Creazione e aggiunta dello stato iniziale all'automa caratteristico
    (stato 0), costituito da un singolo Item LR(0), quello della fresh
    production;
* Calcolo della closure dell'Item contenuto nello stato iniziale con
    aggiunta delle nuove transizioni verso nuovi stati. In questo step
    bisognerà fare attenzione a non aggiungere un nuovo stato se il
    kernel è identico a quello di uno stato già aggiunto in precedenza:
    bisognerà quindi ricercare nell'automa se esiste uno stato con lo
    stesso kernel;
* Per ogni nuovo stato creato si calcola il kernel e lo si aggiunge
    agli items dello stato stesso;
* A partire dal kernel del nuovo stato si calcola la closure e si
    aggiungono le nuove transizioni verso i nuovi stati;
* Si eseguono gli ultimi due punti fino a saturazione, ovvero finché
    ci sono nuovi stati, ovvero stati non marcati nell'automa
    caratteristico;
* Giunti al termine si stampano a video le transizioni e tutte le
    informazioni sui singoli stati dell'automa.

# Strutture dati

Per implementare l'automa caratteristico dato in output dal programma e
la grammatica fornita in input si è cercato di seguire uno stile di
programmazione il più astratto e intuitivo possibile, in modo da rendere
la lettura del codice di facile comprensione. In particolare il
linguaggio C non offrendo delle strutture dati prefabbricate ci ha
obbligati a definirne alcune, sfruttando la keyword del linguaggio
$$struct$$.\
Analizzeremo nei prossimi paragrafi le decisioni implementative prese
per realizzare le varie parti necessarie alla costruzione di un automa
caratteristico. Riportiamo in breve di quali importanti strutture dati
ci avvaleremo durante la trattazione:

1.  Produzione
2.  Grammatica
3.  Item LR(0)
4.  Funzione di transizione
5.  Stato dell'automa caratteristico
6.  Automa caratteristico

## Produzione

La produzione è una struttura dati di base su cui si basa la grammatica,
essa è una struttura minimale composta da 3 parti:
* `char driver` : il driver della produzione ;
* `char[] body` : stringa contenente il body della produzione ;
* `int production_id` : valore intero utilizzato per controllare in
maniera efficiente se le produzioni memorizzate in due item distinti
sono uguali a meno della posizione del marker. Se due produzioni
distinte avranno lo stesso $$production\_id$$ vorrà dire che entrambe
avranno lo stesso body e lo stesso driver.


Riportiamo ora il codice utilizzato per rappresentare una produzione.

``` {.objectivec language="C" style="c"}
struct production {
    char driver;
    char body[MAX_PRODUCTION_BODY_LENGTH];
    
    int production_id; 
};
```

## Grammatica

Il primo step nella procedura di generazione dell'automa caratteristico
è quello di leggere la grammatica fornita in input dall'utente e
memorizzarla. Vista la definizione di $$production$$ fornita in precedenza
definiamo la grammatica come un array di produzioni.\
Ogni nuova produzione sarà inserita nella grammatica sfruttando la
funzione $$addProduction()$$ che si occuperà di rimuovere gli spazi e
dividere le produzioni multi-definite, ovvero le produzioni con lo
stesso driver ma con body differente unite dall'operatore $$|$$ e scritte
sulla stessa riga .\
Riportiamo qui sotto il codice utilizzato per rappresentare la
grammatica.

``` {.objectivec language="C" style="c"}
struct production grammar[MAX_GRAMMAR_PRODUCTIONS_NUMBER];
```

## Item LR(0)

Gli stati dell'automa caratteristico saranno formati da Item LR(0),
ovvero Item associati ad una singola produzione e ad un marker
$$A \to \alpha\cdot\beta$$ Dove il simbolo $$\cdot$$ indica la posizione
del marker all'interno del body della produzione.\
A livello implementativo un modo efficiente per implementare un Item
LR(0) è quello di utilizzare una struct contenente le seguenti
informazioni:
* `production prod` : la produzione associata all'item e definita
sfruttando la definizione di produzione vista in precedenza;
* `int marker_position` : un valore intero rappresentante la posizione del
marker all'interno dell'item;
* `bool isKernelProduction` : settato a true per indicare che un Item fa
parte del Kernel di uno stato.


Riportiamo ora il codice utilizzato per rappresentare gli Item LR(0).

``` {.objectivec language="C" style="c"}
struct lr0_item {
    struct production prod;

    int marker_position;
    bool isKernelProduction;  
};
```

## Funzione di transizione {#tau}

La funzione di transizione associa ad una coppia (stato, letterale) un
altro stato. Se diciamo S gli stati dell'automa caratteristico e V il
vocabolario della grammatica considerata allora definiamo formalmente la
funzione di transizione come $$\tau \colon (S, V) \to S$$

Per poter implementare questa funzione nel modo più semplice ed
efficiente possibile adottiamo una struttura avente 3 attributi:
* `int from` : lo stato origine della transizione ;
* `int to` : lo stato destinazione della transizione ;
* `char by` : un letterale equivalente al valore che leggerò nell'input
buffer. In altre parole questo sarà il letterale che etichetterà l'arco
di transizione.


Riportiamo il codice della struttura associata alla funzione di
transizione.

``` {.objectivec language="C" style="c"}
struct transition {
    int from;
    char by;
    int destination;
};
```

## Stato dell'automa caratteristico

Ogni stato dell'automa caratteristico sarà definito da un identificativo
numerico: la posizione nell'array $$automa$$ definito nella prossima
sezione \"Automa caratteristico\". Descriveremo il singolo stato
attraverso i seguenti attributi:
* `lr0_item items[MAX_STATE_ITEMS]` : dalla definizione sappiamo che ogni
stato dell'automa caratteristico sarà composto da un insieme di LR(0)
items, in questo caso definito tramite un array. Alcuni di questi items
inoltre faranno parte del kernel dello stato.
* `transition transitions[MAX_AUTOMA_STATES_COUNT]` : un insieme di
transizioni uscenti dallo stato corrente e dirette nel verso degli altri
stati. Anche in questo caso sfruttiamo la definizione di
`struct transition` definita in precedenza.
* `state_type type` : indica la tipologia di stato che può assumere un
valore fra i seguenti : $$normale$$, di $$accept$$ oppure $$finale$$.
* `int items_count` : numero totale di items presenti nello stato(compresi
quelli facenti parte del kernel)
* `int kernel_items_count` : numero di items che sono parte del kernel.
Questa variabile è aggiunta al solo scopo di velocizzare la procedura di
controllo della presenza di uno stato con lo stesso kernel nell'automa
caratteristico. Se due stati hanno numero di items del kernel diverso,
sicuramente non potranno essere uguali e quindi si saltano calcoli
aggiuntivi.
* `int transaction_count` : numero di transizioni uscenti dallo stato


L'implementazione dello stato dell'automa caratteristico rispecchia
esattamente i punti appena visti. Riportiamo il codice utilizzato per
rappresentare lo stato dell'automa.

``` {.objectivec language="C" style="c"}
struct automa_state {
    struct lr0_item items[MAX_STATE_ITEMS];  
    struct transition transitions[AUTOMA_STATES_COUNT];
    
    state_type type;

    int items_count;
    int kernel_items_count;
    int transition_count;
};
```

## Automa caratteristico

Siamo giunti all'ultima struttura : l'automa caratteristico per il
parsing bottom-up LR(0). Abbiamo tutte le strutture necessarie per
definirlo. Rappresenteremo l'automa attraverso un insieme di stati
definiti come nel precedente paragrafo.\
Riportiamo per completezza il codice per definire l'automa

``` {.objectivec language="C" style="c"}
struct automa_state automa[MAX_AUTOMA_STATES_COUNT];
```

# Input e Output

## Input

La procedura di generazione dell'automa caratteristico assume che sia
fornita una grammatica in input seguendo una particolari sintassi.
Esistono due modi per fornire la grammatica:

1.  Input da parte dell'utente
2.  Input da file

Entrambi questi metodi assumono che l'input sia fornito in un formato
standard come specificato nel prossimo paragrafo.

### Formato input {#sec:inputformat}

La grammatica fornita dall'utente dovrà rispettare le seguenti regole
sintattiche:

1.  Ogni produzione dovrà essere della forma `A -> α` dove A è un
    non terminale e α è un insieme di simboli terminali e non
    terminali.
2.  driver e body delle produzioni dovranno essere separati da due
    caratteri \"trattino\" e \"maggiore\" formando il simbolo `->`
3.  ogni letterale della grammatica, terminale o non terminale che sia,
    sarà rappresentato da un singolo carattere. Ad esempio se si volesse
    rappresentare il terminale $$digit$$ saremo costretti a utilizzare un
    solo carattere $$d$$. La sequenza $$digit$$ altrimenti verrebbe
    interpretata come 5 terminali
4.  non facendo parte dello standard ascii supportato dal linguaggio C,
    il carattere ε verrà rappresentato dal carattere ~. La
    produzione `A -> ε` verrà quindi rappresentata come
    `A -> ~`
5.  ogni spazio inserito nelle produzioni verrà rimosso in modo da
    evitare che il carattere di spaziatura sia considerato un terminale.

### Input da parte dell'utente

L'utente eseguirà il programma e dopo aver passato lo start symbol della
grammatica come parametro dovrà inserire manualmente ogni singola
produzione separata da un carattere \"a capo\". Per terminare
l'inserimento si dovrà semplicemente cliccare il tasto \"Enter\" dopo
aver inserito l'ultima produzione. Ogni produzione dovrà rispettare lo
standard specificato nella sezione
[4.1.1](#sec:inputformat).

#### Esempio

Supponiamo di avere in input la seguente grammatica avente come start
symbol $$S$$ e di voler generare l'automa caratteristico associato.
$$\begin{aligned}
    S \to aABe \\
    A \to Abc \mid b \\
    B \to d \end{aligned}$$

L'utente eseguirà il programma compilato passando lo start symbol S e le
produzioni una per volta come nel seguente frammento estratto dalla
shell

``` {.objectivec language="C" style="c"}
./automa_generator S
S -> aABe
A -> Abc | b
B -> d

```

### Input da file

In alternativa all'input manuale delle produzioni, l'utente potrà
specificare un file di input contenente la grammatica utilizzando due
metodi diversi:
1.  parametro al programma
2.  ridirezione dell'input

Descriviamo entrambe le alternative

#### 1. Parametro del programma

L'utente fornisce un secondo ulteriore parametro al programma contenente
la posizione del file di input.

##### Esempio

Supponiamo di avere in input la stessa grammatica dell'esempio
precedente per l'input da parte dell'utente. In questo caso però
assumiamo che la grammatica sia memorizzata in un file di testo nella
cartella \"test_grammars\" chiamato \"grammar5.txt\"\
L'utente eseguirà il programma compilato passando lo start symbol S e la
posizione del file come riportato nell'estratto di codice qui sotto

``` {.objectivec language="C" style="c"}
./automa_generator S test-grammars/grammar5.txt
```

#### 2. Ridirezione dell'input

L'utente utilizzando l'operatore di ridirezione $$<$$ fornito dal
linguaggio bash su GNU/Linux ridireziona il contenuto del file
contenente la grammatica allo standard input del programma.

##### Esempio

Anche in questo caso supponiamo di avere in input la grammatica
\"grammar5.txt\" dell'esempio precedente. L'utente eseguirà il programma
compilato passando lo start symbol S e ridezionerà il contenuto del file
della grammatica al programma come nel seguente frammento di codice
estratto dalla shell

``` {.objectivec language="C" style="c"}
./automa_generator S < test-grammars/grammar5.txt
```

## Output

In output il programma ritornerà a video l'automa caratteristico con
tutte le informazioni ad esso annesse. Descriviamo nei prossimi
paragrafi le informazioni restituite in output dall'esecuzione del
programma sulla seguente grammatica: $$\begin{aligned}
    S \to aABe \\
    A \to Abc \mid b \\
    B \to d \end{aligned}$$

``` {.default style="plain"}
Il nuovo fresh Symbol e': K
============== GRAMMATICA ===============
K -> S
S -> aABe
A -> b
A -> Abc
B -> d
============== TRANSIZIONI ==============
Tau (0, S) = 1 
Tau (0, a) = 2 
Tau (2, A) = 3 
Tau (2, b) = 4 
Tau (3, B) = 5 
Tau (3, b) = 6 
Tau (3, d) = 7 
Tau (5, e) = 8 
Tau (6, c) = 9 
=========== ITEMS NEGLI STATI ===========
++++++++++ STATO 0 
K -> .S		[ K ]
S -> .aABe
++++++++++ STATO 1 Stato di accept
K -> S.		[ K ]
++++++++++ STATO 2 
S -> a.ABe	[ K ]
A -> .b
A -> .Abc
++++++++++ STATO 3 
S -> aA.Be	[ K ]
A -> A.bc	[ K ]
B -> .d
++++++++++ STATO 4 Stato finale
A -> b.		[ K ]
++++++++++ STATO 5 
S -> aAB.e	[ K ]
++++++++++ STATO 6 
A -> Ab.c	[ K ]
++++++++++ STATO 7 Stato finale
B -> d.		[ K ]
++++++++++ STATO 8 Stato finale
S -> aABe.	[ K ]
++++++++++ STATO 9 Stato finale
A -> Abc.	[ K ]
```

Come si evince dal listato sopra, l'output sarà diviso in 3 parti:

1.  Grammatica: la prima parte conterrà la grammatica letta in input con
    aggiunta la fresh production generata a partire dal nuovo fresh
    symbol(specificato a linea 1) e dallo start symbol passato come
    parametro.

    #### NOTA:

    il fresh symbol viene calcolato in automatico a partire da una lista
    di non terminali (lettere dell'alfabeto da A a Z, solitamente la
    scelta ricade sulla lettera K in quanto le lettere A, B e C sono
    spesso presenti nelle grammatiche che abbiamo visto, riducendo il
    numero di tentativi necessari per cercare un nuovo non terminale).

2.  Transizioni : la seconda parte contiene una lista di transizioni dai
    vari stati, rispettando la definizione di funzione di transizione
    vista nella sezione [3.4](#tau).

    #### Esempio: 

    `Tau(0,S) = 1` indica una transizione dallo stato 0 allo stato 1
    tramite il non terminale S.

3.  Items degli stati: per ogni stato dell'automa caratteristico vengono
    stampati il valore identificativo dello stato, il tipo di stato e i
    rispettivi items che lo compongono. Per ogni item appartenente ad
    uno stato, se l'item fa parte del kernel presenterà un simbolo `[K]`
    sulla destra.\
    Il tipo di stato può assumere un valore fra i seguenti:

# Testing

Per testare l'effettivo funzionamento dell'algoritmo si è deciso di
procedere eseguendo dei test di difficoltà incrementale, partendo da
grammatiche semplici composte da poche produzioni fino ad arrivare a
grammatiche costituite da più produzioni con lo stesso driver separate
da $$\mid$$ e con l'aggiunta di produzioni con body uguale a ε.\
Nella directory `test_grammars` sono presenti alcune grammatiche
d'esempio viste a lezione. Queste ultime sono state testate una ad una
producendo dei risultati corretti e in linea con gli automi
caratteristici generati a lezione, a meno dell'ordinamento di
assegnazione dei nomi agli stati. Riportiamo in seguito il risultato
dell'esecuzione su alcune di esse.

## Grammatica 1

La prima grammatica che testeremo è una grammatica molto semplice
costituita da 4 produzioni. In questo caso vediamo in azione le
potenzialità dell'operatore di unione $$\mid$$ il quale ci permette di
scrivere più produzioni con lo stesso driver sulla stessa riga.\
Riportiamo per completezza la grammatica in questione:

``` {.default style="plain"}
S -> aABe
A -> Abc | b
B -> d
```

L'esecuzione del programma su questa grammatica genera in output
l'automa caratteristico descritto secondo il formato specificato nella
sezione dedicata all'interpretazione dell'output(Sezione
[4.2](#output)).

``` {.default style="plain"}
Il nuovo fresh Symbol e': K
============== GRAMMATICA ===============
K -> S
S -> aABe
A -> b
A -> Abc
B -> d
============== TRANSIZIONI ==============
Tau (0, S) = 1 
Tau (0, a) = 2 
Tau (2, A) = 3 
Tau (2, b) = 4 
Tau (3, B) = 5 
Tau (3, b) = 6 
Tau (3, d) = 7 
Tau (5, e) = 8 
Tau (6, c) = 9 
=========== ITEMS NEGLI STATI ===========
++++++++++ STATO 0 
K -> .S		[ K ]
S -> .aABe
++++++++++ STATO 1 Stato di accept
K -> S.		[ K ]
++++++++++ STATO 2 
S -> a.ABe	[ K ]
A -> .b
A -> .Abc
++++++++++ STATO 3 
S -> aA.Be	[ K ]
A -> A.bc	[ K ]
B -> .d
++++++++++ STATO 4 Stato finale
A -> b.		[ K ]
++++++++++ STATO 5 
S -> aAB.e	[ K ]
++++++++++ STATO 6 
A -> Ab.c	[ K ]
++++++++++ STATO 7 Stato finale
B -> d.		[ K ]
++++++++++ STATO 8 Stato finale
S -> aABe.	[ K ]
++++++++++ STATO 9 Stato finale
A -> Abc.	[ K ]
```

Automa caratteristico per la Grammatica in questione 
![automa 1](https://github.com/FabrizioSandri/LR-0-automa-generator/blob/main/Latex/assets/automa1.png?raw=true)

## Grammatica 2

La seconda grammatica descrive della banali operazioni di somma e
moltiplicazione. Riportiamo la grammatica in questione:

``` {.default style="plain"}
E -> E+E | E*E | i
```

L'esecuzione del programma su questa grammatica genera in output
l'automa caratteristico descritto secondo il formato specificato nella
sezione dedicata all'interpretazione dell'output(Sezione
[4.2](#output)).

``` {.default style="plain"}
Il nuovo fresh Symbol e': K
============== GRAMMATICA ===============
K -> E
E -> i
E -> E*E
E -> E+E
============== TRANSIZIONI ==============
Tau (0, E) = 1 
Tau (0, i) = 2 
Tau (1, *) = 3 
Tau (1, +) = 4 
Tau (3, E) = 5 
Tau (3, i) = 2 
Tau (4, E) = 6 
Tau (4, i) = 2 
Tau (5, *) = 3 
Tau (5, +) = 4 
Tau (6, *) = 3 
Tau (6, +) = 4 
=========== ITEMS NEGLI STATI ===========
++++++++++ STATO 0 
K -> .E		[ K ]
E -> .i
E -> .E*E
E -> .E+E
++++++++++ STATO 1 Stato di accept
K -> E.		[ K ]
E -> E.*E	[ K ]
E -> E.+E	[ K ]
++++++++++ STATO 2 Stato finale
E -> i.		[ K ]
++++++++++ STATO 3 
E -> E*.E	[ K ]
E -> .i
E -> .E*E
E -> .E+E
++++++++++ STATO 4 
E -> E+.E	[ K ]
E -> .i
E -> .E*E
E -> .E+E
++++++++++ STATO 5 Stato finale
E -> E*E.	[ K ]
E -> E.*E	[ K ]
E -> E.+E	[ K ]
++++++++++ STATO 6 Stato finale
E -> E+E.	[ K ]
E -> E.*E	[ K ]
E -> E.+E	[ K ]
```

Automa caratteristico per la Grammatica in questione
![automa 2](https://github.com/FabrizioSandri/LR-0-automa-generator/blob/main/Latex/assets/automa2.png?raw=true)

## Grammatica 3

La terza grammatica è simile alla grammatica 3 ma con l'aggiunta delle
parentesi e un operatore di sottrazione al posto di quello di
moltiplicazione. Sarà ora possibile combinare espressioni multiple e
separarle utilizzando delle parentesizzazioni. Riportiamo per
completezza la grammatica in questione:

``` {.default style="plain"}
E -> E + T  | E - T | T
T -> (E) | i | n
```

L'esecuzione del programma su questa grammatica genera in output
l'automa caratteristico descritto nel seguente listato

``` {.default style="plain"}
Il nuovo fresh Symbol e': K
============== GRAMMATICA ===============
K -> E
E -> T
E -> E-T
E -> E+T
T -> n
T -> i
T -> (E)
============== TRANSIZIONI ==============
Tau (0, E) = 1 
Tau (0, T) = 2 
Tau (0, n) = 3 
Tau (0, i) = 4 
Tau (0, () = 5 
Tau (1, -) = 6 
Tau (1, +) = 7 
Tau (5, E) = 8 
Tau (5, T) = 2 
Tau (5, n) = 3 
Tau (5, i) = 4 
Tau (5, () = 5 
Tau (6, T) = 9 
Tau (6, n) = 3 
Tau (6, i) = 4 
Tau (6, () = 5 
Tau (7, T) = 10 
Tau (7, n) = 3 
Tau (7, i) = 4 
Tau (7, () = 5 
Tau (8, )) = 11 
Tau (8, -) = 6 
Tau (8, +) = 7 
=========== ITEMS NEGLI STATI ===========
++++++++++ STATO 0 
K -> .E		[ K ]
E -> .T
E -> .E-T
E -> .E+T
T -> .n
T -> .i
T -> .(E)
++++++++++ STATO 1 Stato di accept
K -> E.		[ K ]
E -> E.-T	[ K ]
E -> E.+T	[ K ]
++++++++++ STATO 2 Stato finale
E -> T.		[ K ]
++++++++++ STATO 3 Stato finale
T -> n.		[ K ]
++++++++++ STATO 4 Stato finale
T -> i.		[ K ]
++++++++++ STATO 5 
T -> (.E)	[ K ]
E -> .T
E -> .E-T
E -> .E+T
T -> .n
T -> .i
T -> .(E)
++++++++++ STATO 6 
E -> E-.T	[ K ]
T -> .n
T -> .i
T -> .(E)
++++++++++ STATO 7 
E -> E+.T	[ K ]
T -> .n
T -> .i
T -> .(E)
++++++++++ STATO 8 
T -> (E.)	[ K ]
E -> E.-T	[ K ]
E -> E.+T	[ K ]
++++++++++ STATO 9 Stato finale
E -> E-T.	[ K ]
++++++++++ STATO 10 Stato finale
E -> E+T.	[ K ]
++++++++++ STATO 11 Stato finale
T -> (E).	[ K ]
```
Automa caratteristico per la Grammatica in questione
![automa 3](https://github.com/FabrizioSandri/LR-0-automa-generator/blob/main/Latex/assets/automa3.png?raw=true)

## Grammatica 6

Riportiamo l'output della Grammatica 6, tralasciando la grammatica 4 e
la grammatica dei puntatori 5 per non appesantire troppo il report: il
lettore può eseguire in autonomia il programma per la generazione
dell'automa caratteristico applicato alle grammatiche suddette.\
La grammatica 6 viene riportata in questo report in quanto contiene
delle ε-produzioni e come specificato nelle sezione
[4.1.1](#sec:inputformat) dedicata al Formato dell'Input le
ε-produzioni dovranno essere scritte utilizzando il simbolo ~.\
Riportiamo la grammatica in questione:

``` {.default style="plain"}
S -> AaB | b
A -> BcBaA | ~
B -> ~
```

Come possiamo vedere le produzioni `A -> ε` `B -> ε` sono 
state riscritte rispettivamente come `A -> ~` `B -> ~`

L'esecuzione genererà l'output del seguente listato

``` {.default style="plain"}
Il nuovo fresh Symbol e': K
============== GRAMMATICA ===============
K -> S
S -> b
S -> AaB
A -> ~
A -> BcBaA
B -> ~
============== TRANSIZIONI ==============
Tau (0, S) = 1 
Tau (0, b) = 2 
Tau (0, A) = 3 
Tau (0, B) = 4 
Tau (3, a) = 5 
Tau (4, c) = 6 
Tau (5, B) = 7 
Tau (6, B) = 8 
Tau (8, a) = 9 
Tau (9, A) = 10 
Tau (9, B) = 4 
=========== ITEMS NEGLI STATI ===========
++++++++++ STATO 0 Stato finale
K -> .S		[ K ]
S -> .b
S -> .AaB
A -> .
A -> .BcBaA
B -> .
++++++++++ STATO 1 Stato di accept
K -> S.		[ K ]
++++++++++ STATO 2 Stato finale
S -> b.		[ K ]
++++++++++ STATO 3 
S -> A.aB	[ K ]
++++++++++ STATO 4 
A -> B.cBaA	[ K ]
++++++++++ STATO 5 Stato finale
S -> Aa.B	[ K ]
B -> .
++++++++++ STATO 6 Stato finale
A -> Bc.BaA	[ K ]
B -> .
++++++++++ STATO 7 Stato finale
S -> AaB.	[ K ]
++++++++++ STATO 8 
A -> BcB.aA	[ K ]
++++++++++ STATO 9 Stato finale
A -> BcBa.A	[ K ]
A -> .
A -> .BcBaA
B -> .
++++++++++ STATO 10 Stato finale
A -> BcBaA.	[ K ]
```

Automa caratteristico per la Grammatica in questione 
![automa 6](https://github.com/FabrizioSandri/LR-0-automa-generator/blob/main/Latex/assets/automa6.png?raw=true)

## Grammatica 7

Vista la dimensione della grammatica numero 7 (presente nella cartella
`test_grammars`) e dell'automa caratteristico generato a partire da
essa, non riportiamo l'output generato dall'algoritmo di generazione
dell'automa caratteristico.\
L'esecuzione porta alla generazione di un'automa caratteristico avente
38 stati e un totale di 97 transizioni.

# Limiti

Come accennato nelle precedenti sezioni di questo documento il programma
è limitato dal fatto che ogni elemento del vocabolario, terminale o non
terminale che sia, deve essere composto da un solo carattere. Ad esempio
per rappresentare il terminale `id` abbiamo dovuto ricorrere
all'utilizzo del singolo carattere `i`.

Vista questa limitazione è apparso subito inutile l'utilizzo di
strutture dati di dimensione dinamica allocate in nella memoria heap.
Per questo si è invece optato per mantenere il tutto nella memoria stack
e porre delle limitazioni riguardo a:
* numero massimo di stati dell'automa caratteristico (100 stati)
* numero massimo di produzioni di una grammatica (100 produzioni)
* lunghezza massima di una produzione in caratteri (50 caratteri)
