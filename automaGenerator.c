#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_GRAMMAR_PRODUCTIONS_NUMBER 50
#define PRODUCTION_BODY_LENGTH 50
#define PRODUCTION_LENGTH 50
#define MAX_AUTOMA_STATES_COUNT 100

#define EPSILON '~'  // il carattere specificato e' un alias per il carattere '\epsilon'

typedef enum { false, true } bool;
typedef enum { normal, accept, final } state_type;

struct production {
    char driver;
    char body[PRODUCTION_BODY_LENGTH];
    int production_id; // utilizzato per controllare velocemente se una produzione e' uguale ad un altra senza controllare body e driver
};

struct lr0_item {
    struct production prod;

    int marker_position;
    bool isKernelProduction;  // utilizzata per capire se un item fa parte del kernel
};

struct transition {
    int from;
    char by;
    int destination;
};

struct automa_state {
    struct lr0_item items[50];  
    struct transition transitions[MAX_AUTOMA_STATES_COUNT];
    
    state_type type;

    int items_count;
    int kernel_items_count;
    int transition_count;
};

bool isNonTerminal(char val){
    if (val >= 'A' && val <= 'Z'){
        return true;
    }
    return false;
}


/**
* Rimuove gli spazi da una stringa, in questo caso da una produzione
*/
void removeSpaces(char* production){
    int resultLen = 0;

    for (int i=0; i<strlen(production); i++){
        if (production[i] != ' '){
            production[resultLen++] = production[i];
        }
    }
    production[resultLen] = '\0';

}

/*
* Funzione di supporto utilizzata per aggiungere una produzione alla grammatica data
* 
* Parametri:
*  - grammar : la grammatica in cui inserire la nuova produzione
*  - production : la produzione da inserire
*  - productions_count : il numero di produzioni aggiunte fin'ora
* 
* Ritorna:
*  - true : produzione inserita con successo
*  - false : errore, la produzione non e'stata inserita in quanto non rispetta lo standard A -> beta
*/
bool addProduction(struct production* grammar, char* new_production, int* productions_count){
    // split della produzione in driver e body
    int bodyStartPosition;
    bool foundArrow = false;
    
    // rimozione degli spazi
    removeSpaces(new_production);

    // ricerca di produzioni multi-defined (separate dalla | )
    char newSeparatedProduction[PRODUCTION_LENGTH] = "_ ->";
    char* separatorOccurence = strchr(new_production, '|');

    if(separatorOccurence){
        separatorOccurence[0] = '\0'; // sostituisci la | con il carattere di fine stringa nella produzione originale
        newSeparatedProduction[0] = new_production[0]; // copia del driver

        strcat(newSeparatedProduction, separatorOccurence + 1); // aggiunta del body

        addProduction(grammar, newSeparatedProduction, productions_count);
    }

    // ricerca del sibolo arrow "->""
    for (bodyStartPosition=1; bodyStartPosition<strlen(new_production); bodyStartPosition++){
        if (new_production[bodyStartPosition-1] == '-' && new_production[bodyStartPosition] == '>' ){
            bodyStartPosition++;
            foundArrow = true;
            break;
        }
    }

    // se e' una produzione valida
    if (foundArrow && isNonTerminal(new_production[0])){
        
        // Aggiungi la produzione alla grammatica separando driver e body
        grammar[*productions_count].driver = new_production[0];
        strcpy(grammar[*productions_count].body, (new_production + bodyStartPosition));
        grammar[*productions_count].production_id = *productions_count; 
        
        (*productions_count)++;

        return true;
    }

    return false;
}

/**
* cambia il fresh symbol della grammatica nel caso in cui vi fosse un conflitto 
*/
void updateFreshSymbol(struct production* grammar, int productions_count){
    bool foundConflict = false;
    char newFreshSymbol = 0;
    char* freshSymbols = "KABCDEFGHIJLMNOPQRSTUVWXYZ"; // possibili fresh symbols

    do {
        foundConflict = false;

        for(int i=1; i<productions_count; i++){ //da 1 in quanto si evita di controllare la fresh production (la prima inserita nella grammatica)
            if (grammar[i].driver == freshSymbols[newFreshSymbol]){ // fresh symbol che non va bene, ne cerco un altro
                foundConflict = true;
                newFreshSymbol++;
                break;
            }
        }

    }while(foundConflict);

    // aggiorna il driver dell'entry point con il fresh symbol
    grammar[0].driver = freshSymbols[newFreshSymbol];
    printf("Il nuovo fresh Symbol e': %c\n", grammar[0].driver);
    
}

/**
* destination e' lo stato di destinazione delle closure da aggiungere
*/
void addItemToClosure(struct automa_state* destinationState, struct lr0_item* item){
    int itemsInDestination = destinationState->items_count;

    // controllo che l'item non sia gia presente nella closure. Un elemento e gia presente nella closure se ha lo stesso identificativo (stesso driver e stesso body)
    // e se la posizione del marker del nuovo item da inserire e' pari a 0
    bool alreadyIn = false;
    for (int i=0; i<itemsInDestination; i++){
        if (destinationState->items[i].prod.production_id == item->prod.production_id && destinationState->items[i].marker_position == 0 ){
            alreadyIn = true;
            break;
        }
    }

    if (alreadyIn == false){ // aggiungo solo se non e' gia presente
        destinationState->items[itemsInDestination] = *item;
        destinationState->items[itemsInDestination].marker_position = 0;
        destinationState->items[itemsInDestination].isKernelProduction = false;

        destinationState->items_count++;
    }
    
}

// funzione utilizzata per aggiungere una produzione al kernel di uno stato (inizializza il kernel a partire dallo stato precedente item)
void addItemToKernel(struct automa_state* destinationState, struct lr0_item* item){
    int itemsInDestination = destinationState->items_count;

    // controllo che l'item non sia gia presente nella closure. Un elemento e gia presente nella closure se ha lo stesso identificativo (stesso driver e stesso body)
    // e se la posizione del marker e' la stessa + 1 (la produzione aggiunta avra il marker spostato in avanti di una posizione)
    bool alreadyIn = false;
    for (int i=0; i<itemsInDestination; i++){
        if (destinationState->items[i].prod.production_id == item->prod.production_id && destinationState->items[i].marker_position == item->marker_position + 1 ){
            alreadyIn = true;
            break;
        }
    }

    if (alreadyIn == false){ // aggiungo solo se non e' gia presente
        destinationState->items[itemsInDestination] = *item;
        destinationState->items[itemsInDestination].marker_position = item->marker_position + 1;
        destinationState->items[itemsInDestination].isKernelProduction = true;

        destinationState->items_count++;
        destinationState->kernel_items_count++;
    }
    
}


void computeClosure(struct automa_state* state, struct production* grammar, int productions_count){
    int unmarkedItemId = 0;
    while( unmarkedItemId < state->items_count ){
        // se contine un marker prima di un non terminale si fa la closure
        int dot = state->items[unmarkedItemId].marker_position;
        char nextToDot = state->items[unmarkedItemId].prod.body[dot];
        if (isNonTerminal(nextToDot)){
            for(int t=0; t<productions_count; t++){
                if (grammar[t].driver == nextToDot){
                    struct lr0_item newItem = {
                        .prod = grammar[t],
                        .marker_position = 0,
                        .isKernelProduction = false
                    };
                    addItemToClosure(state, &newItem);
                }
            }
        }
        unmarkedItemId++;
    }
    
}

/**
* Questa funzione permette di controllare se il kernel generato da uno stato "state" proseguendo con una transizione 
* tramite "nextChar" e' gia presente tra quelli presenti nell'automa caratteristico. 
* Se lo stato e' gia presente si ritorna l'identificativo dello stato
*
* Parametri:
* - automa : l'automa caratteristico finale
* - totalStates : numero di stati dell'automa caratteristico costruito fin'ora
* - stateId : l'id dello stato sorgente. Si controllera' se questo stato e' gia presente nell'automa
* - nextChar : il prossimo carattere della transizione (terminale o non terminale)
*
* Ritorna:
* - id_stato : se si e' trovato uno stato con id stato_id e con kernel uguale gia presente 
* - -1 : se non esiste ancora uno stato con quel kernel
*/
int getKernelEqualTo(struct automa_state* automa, int totalStates, int stateId, char nextChar){
    int kernelEqualTo = -1;

    struct lr0_item kernelOfState[MAX_GRAMMAR_PRODUCTIONS_NUMBER];
    int stateIdKernelSize = 0;  // la dimensione del nuovo kernel effettuando una transizione con nextChar a partire da stateId

    // estrai le produzione che faranno parte del kernel del nuovo stato partendo da stateId
    for (int i=0; i<automa[stateId].items_count; i++){
        int marker_pos = automa[stateId].items[i].marker_position;
        char productionNextChar = automa[stateId].items[i].prod.body[marker_pos];
        
        if (productionNextChar == nextChar){
            kernelOfState[stateIdKernelSize++] = automa[stateId].items[i];
        }
    }

    // cerca se il kernel del futuro nuovo stato e' gia presente nell'automa. Nel caso fosse gia presente ritorniamo subito l'id dello stato uguale
    for (int t=0; t<totalStates; t++){
        
        if (stateIdKernelSize == automa[t].kernel_items_count){ // affinche' due kernel siano uguali devono avere come minimo lo stesso numero di produzioni
            bool allEqual = true; 
        
            for (int kernelProd=0; kernelProd<stateIdKernelSize; kernelProd++){
                bool corrispondenza = false;
                for (int prod=0; prod<automa[t].items_count; prod++){
                    if (automa[t].items[prod].prod.production_id == kernelOfState[kernelProd].prod.production_id && automa[t].items[prod].marker_position == kernelOfState[kernelProd].marker_position+1 && automa[t].items[prod].isKernelProduction){
                        corrispondenza = true; // ho trovato una produzione dell'automa che matcha con quella del kernel  
                    }
                }

                if (corrispondenza == false){  // nessuna corrispondenza per questa produzione del kernel, posso terminare, sicuramente e' uno stato nuovo
                    allEqual = false;
                    break;
                }

            }


            if (allEqual == true){ // trovato
                kernelEqualTo = t;
                break;
            }

        }
        
    }
    
    return kernelEqualTo;
}


/**
* Generazione dell'automa caratteristico,
* Ritorna: il numero di stati dell'automa caratteristico
*/
int generateAutomaChar(struct automa_state* automa, struct production* grammar, int productions_count, char startSymbol){
    int totalStates = 0; 

    // le seguenti due variabili servono per tenere traccia del numero di nuovi stati aggiunti a partire da ogni stato :
    // in particolare serviranno per evitare di creare un nuovo stato se esistesse gia
    char alreadyAddedNextChar[MAX_AUTOMA_STATES_COUNT];

    //////////////// INIZIALIZZAZIONE /////////////////
    // aggiunta all'automa dello stato 0 con il suo kernel
    struct automa_state state0 = {
        .items_count = 0,
        .kernel_items_count = 0,
        .transition_count = 0,
        .type = normal
    };


    // aggiunta dell'item del fresh symbol al kernel dello stato iniziale 0
    struct lr0_item freshSymbolKernel = {
        .prod = grammar[0],
        .marker_position = 0,
        .isKernelProduction = false
    };

    addItemToKernel(&state0, &freshSymbolKernel); 
    automa[totalStates] = state0;
    automa[totalStates].items[0].marker_position = 0;  // reset del marker alla posizione 0 in quanto addItemToKernel sposta il marker in avanti di 1
    totalStates++;
    
    /////////////// SVOLGIMENTO //////////////////
    int unmarkedStateId = 0;
    while ( unmarkedStateId < totalStates) {   // finche esiste uno stato unmarked
         
        // closure del kernel dello stato unmarked (uno stato unmarked contiene solo il kernel)       
        computeClosure(&automa[unmarkedStateId], grammar, productions_count);
        
        for (int i=0; i<automa[unmarkedStateId].items_count; i++){ // aggiunta delle transizioni a partire dagli stati unmarked

            struct lr0_item item = automa[unmarkedStateId].items[i];

            int marker_pos = item.marker_position;
            if (marker_pos < strlen(item.prod.body) && item.prod.body[marker_pos] != EPSILON){ // se il marker non è in ultima posizione e non e' una transizione tramite epsilon
                char nextChar = item.prod.body[marker_pos];

                // alreadyAddedStateId conterra' l'id di uno stato gia esistente se per quel nextChar lo stato e' gia stato creato
                int alreadyAddedStateId = -1;
                for (int tr=0; tr<automa[unmarkedStateId].transition_count; tr++){
                    if (nextChar == automa[unmarkedStateId].transitions[tr].by){ // lo stato verso questo carattere e' gia stato inserito
                        alreadyAddedStateId = automa[unmarkedStateId].transitions[tr].destination; 
                        break;
                    }
                }


                int kernelEqualTo = -1;
                if (alreadyAddedStateId != -1){ // se lo stato destinazione esiste gia allora aggiungo l'item al kernel 
                    addItemToKernel(&automa[alreadyAddedStateId], &item); 
                }else if ( (kernelEqualTo = getKernelEqualTo(automa, totalStates, unmarkedStateId, nextChar)) != -1){ // kernel gia presente, aggiungi solo la transizione verso lo stato specificato da kernelEqualTo

                    struct transition newTransition = {
                        .from = unmarkedStateId,
                        .by = item.prod.body[marker_pos],  // il prossimo simbolo
                        .destination = kernelEqualTo
                    };

                    
                    printf("Tau (%d, %c) = %d \n", newTransition.from, newTransition.by, newTransition.destination);
                    automa[unmarkedStateId].transitions[automa[unmarkedStateId].transition_count++] = newTransition;

                }else{ // nuovo stato

                    struct transition newTransition = {
                        .from = unmarkedStateId,
                        .by = item.prod.body[marker_pos],  // il prossimo simbolo
                        .destination = totalStates
                    };

                    
                    printf("Tau (%d, %c) = %d \n", newTransition.from, newTransition.by, newTransition.destination);
                    automa[unmarkedStateId].transitions[automa[unmarkedStateId].transition_count++] = newTransition;

                    // creazione e aggiunta del nuovo stato
                    struct automa_state newState = {
                        .items_count = 0,
                        .kernel_items_count = 0,
                        .transition_count = 0,
                        .type = normal
                    };

                    addItemToKernel(&newState, &item); // aggiunta del kernel al nuovo stato
                    automa[totalStates++] = newState;

                }

                
            }else{ // marker in ultima posizione : reducing item (mark dello stato come stato finale oppure accept)
                if (item.prod.body[marker_pos - 1] == startSymbol && item.prod.driver==automa[0].items[0].prod.driver){
                    automa[unmarkedStateId].type = accept;
                }else{
                    automa[unmarkedStateId].type = final;
                }
            }

        }

        unmarkedStateId++;
    }

    return totalStates;

}


int main(int argc, char** argv){
    FILE* inputSource = stdin;
    struct production grammar[MAX_GRAMMAR_PRODUCTIONS_NUMBER];
    
    char startSymbol;

    char new_production[PRODUCTION_LENGTH];
    int productions_count = 0;
    int totalStates;

    if (argc < 2) {
        printf("Use %s <start_symbol> <grammar_file>\n", argv[0]);
        exit(0);
    }
    startSymbol = argv[1][0];
    
    if (argc == 3){ // file della grammatica in input
        inputSource = fopen(argv[2], "r");
    }

    // estendi la grammatica P a P' : grammatica con aggiunta la produzione K -> startSymbol. K deve essere un fresh symbol.
    // Si controlla che K sia un fresh symbol utilizzando la funzione updateFreshSymbol dopo aver letto tutte le produzioni possibili.
    char fresh_production[] = "K -> _";
    fresh_production[5] = startSymbol;
    addProduction(grammar, fresh_production, &productions_count);

    // leggo le produzioni una ad una
    while (fgets(new_production, PRODUCTION_LENGTH, inputSource) && new_production[0] != '\n'){
        // rimuovo il carattere newline
        new_production[strlen(new_production) - 1] = '\0';  

        if (addProduction(grammar, new_production, &productions_count) == false){ 
            printf("La produzione %s non e' stata inserita in quanto non rispetta lo standard: A -> beta\n", new_production);
        }

    }

    updateFreshSymbol(grammar, productions_count);

    /////////////////////// STAMPA PRODUZIONI LETTE ///////////////////////
    printf("============== GRAMMATICA ===============\n");
    for (int i=0; i<productions_count; i++){
        printf("%c -> %s\n", grammar[i].driver, grammar[i].body);
    }

    ////////////////////////// CREAZIONE AUTOMA //////////////////////////

    printf("============== TRANSIZIONI ==============\n");
    struct automa_state automa[MAX_AUTOMA_STATES_COUNT];
    totalStates = generateAutomaChar(automa, grammar, productions_count, startSymbol);


    /////////////////////////////// STAMPA //////////////////////////////
    printf("=========== ITEMS NEGLI STATI ===========\n");
    for(int state=0; state<totalStates; state++){
        char* state_type = "";
        if (automa[state].type == accept){
            state_type = "Stato di accept";
        }else if(automa[state].type == final){
            state_type = "Stato finale";
        }

        printf("++++++++++ STATO %d %s\n", state, state_type);
        for(int itemId = 0; itemId < automa[state].items_count; itemId++){
            struct lr0_item* item = &automa[state].items[itemId];
            printf("%c -> ", item->prod.driver);
            if (item->prod.body[0] == EPSILON){
                printf(".");
            }else{
                for (int t=0; t<strlen(item->prod.body); t++){
                    if (t == item->marker_position){
                        printf(".%c",item->prod.body[t]);
                    }else{
                        printf("%c",item->prod.body[t]);
                    }
                }
            }
            
            if (item->marker_position == strlen(item->prod.body)){ // marker in ultima posizione
                printf(".");
            }

            if (item->isKernelProduction){ // item facente parte del kernel
                if (strlen(item->prod.body) == 1)
                    printf("\t");
                printf("\t[ K ]");
            }
            
            printf("\n");
        }
        
    }
            
    return(0);
}
