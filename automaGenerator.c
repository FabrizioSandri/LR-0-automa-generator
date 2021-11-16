#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_GRAMMAR_PRODUCTIONS_NUMBER 20
#define MAX_PRODUCTION_BODY_LENGTH 30

typedef enum { false, true } bool;

struct lr0_item {
    char driver;
    char body[MAX_PRODUCTION_BODY_LENGTH];

    int marker_position;
    bool marked;
    int production_id; // utilizzato per controllare velocemente se una produzione e' uguale ad un altra senza controllare body e driver
    bool isKernelProduction;  // utilizzata per capire se un item fa parte del kernel
};

struct transaction {
    char from;
    char by;
    char destination;
};

struct automa_state {
    struct lr0_item items[50];  
    struct transaction transactions[50];
    bool marked;
    
    int items_count;
    int kernel_items_count;
    int transaction_count;
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
bool addProduction(struct lr0_item* grammar, char* production, int* productions_count){
    // split della produzione in driver e body
    int bodyStartPosition;
    bool foundArrow = false;
    
    // rimozione degli spazi
    removeSpaces(production);

    // ricerca di produzioni multi-defined (separate dalla | )
    char newSeparatedProduction[MAX_PRODUCTION_BODY_LENGTH + 10] = "_ ->";
    char* separatorOccurence = strchr(production, '|');

    if(separatorOccurence){
        separatorOccurence[0] = '\0'; // sostituisci la | con il carattere di fine stringa nella produzione
        newSeparatedProduction[0] = production[0]; // copia del driver

        strcat(newSeparatedProduction, separatorOccurence + 1); // aggiunta del body

        addProduction(grammar, newSeparatedProduction, productions_count);
    }

    // ricerca del sibolo arrow "->""
    for (bodyStartPosition=1; bodyStartPosition<strlen(production); bodyStartPosition++){
        if (production[bodyStartPosition-1] == '-' && production[bodyStartPosition] == '>' ){
            bodyStartPosition++;
            foundArrow = true;
            break;
        }
    }

    // se e' una produzione valida
    if (foundArrow && isNonTerminal(production[0])){
        
        // Aggiungi la produzione alla grammatica separando driver e body
        grammar[*productions_count].driver = production[0];
        strcpy(grammar[*productions_count].body, (production + bodyStartPosition));

        grammar[*productions_count].marker_position = 0; 
        grammar[*productions_count].marked = false; 
        grammar[*productions_count].production_id = *productions_count; 
        grammar[*productions_count].isKernelProduction = false; 
        

        (*productions_count)++;

        return true;

    }

    return false;
}

/**
* cambia il fresh symbol della grammatica nel caso in cui vi fosse un conflitto 
*/
void updateFreshSymbol(struct lr0_item* grammar, int productions_count){
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
* conta il numero di produzioni targate come unmarked all'interno degli items dello stato state
*/
int countUnmarked(struct automa_state* state){
    int count = 0;

    for (int i=0; i<state->items_count; i++){
        if (state->items[i].marked == false){
            count ++;
        }
    }

    return count;
}

/**
* ottiene l'id di uno stato unMarked all'interno dell'automa specificato
*/
int getUnmarkedStateId(struct automa_state* automa, int numberOfStates){
    int id = -1;

    for (int i=0; i<numberOfStates; i++){
        if (automa[i].marked == false){
            id = i;
            break;
        }
    }

    return id;
}

/**
* destination e' lo stato di destinazione delle closure da aggiungere
*/
void addProductionToClosure(struct automa_state* destinationState, struct lr0_item item){
    int itemsInDestination = destinationState->items_count;

    // controllo che l'item non sia gia presente nella closure. Un elemento e gia presente nella closure se ha lo stesso identificativo (stesso driver e stesso body)
    // e se la posizione del marker e' la stessa
    bool alreadyIn = false;
    for (int i=0; i<itemsInDestination; i++){
        if (destinationState->items[i].production_id == item.production_id && destinationState->items[i].marker_position == item.marker_position ){
            alreadyIn = true;
            break;
        }
    }

    if (alreadyIn == false){ // aggiungo solo se non e' gia presente
        destinationState->items[itemsInDestination] = item;
        destinationState->items[itemsInDestination].marked = false;
        destinationState->items[itemsInDestination].marker_position = 0;
        destinationState->items[itemsInDestination].isKernelProduction = false;

        destinationState->items_count++;
    }
    
}

// funzione utilizzata per aggiungere una produzione al kernel di uno stato (inizializza il kernel a partire dallo stato precedente item)
void addProductionToKernel(struct automa_state* destinationState, struct lr0_item item){
    int itemsInDestination = destinationState->items_count;

    // controllo che l'item non sia gia presente nella closure. Un elemento e gia presente nella closure se ha lo stesso identificativo (stesso driver e stesso body)
    // e se la posizione del marker e' la stessa
    bool alreadyIn = false;
    for (int i=0; i<itemsInDestination; i++){
        if (destinationState->items[i].production_id == item.production_id && destinationState->items[i].marker_position == item.marker_position ){
            alreadyIn = true;
            break;
        }
    }

    if (alreadyIn == false){ // aggiungo solo se non e' gia presente
        destinationState->items[itemsInDestination] = item;
        destinationState->items[itemsInDestination].marked = false;
        destinationState->items[itemsInDestination].marker_position = item.marker_position + 1;
        destinationState->items[itemsInDestination].isKernelProduction = true;

        destinationState->items_count++;
        destinationState->kernel_items_count++;
    }
    
}


void computeClosure(struct automa_state* state, struct lr0_item* grammar, int productions_count){

    while(countUnmarked(state) != 0){

        for (int i=0; i<state->items_count; i++){
            if (state->items[i].marked == false){
                state->items[i].marked = true;

                // se contine un marker prima di un non terminale si fa la closure
                int dot = state->items[i].marker_position;
                char nextToDot = state->items[i].body[dot];
                if (isNonTerminal(nextToDot)){
                    for(int t=0; t<productions_count; t++){
                        if (grammar[t].driver == nextToDot){
                            addProductionToClosure(state, grammar[t]);
                        }
                    }
                }

            }
        }
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
        char productionNextChar = automa[stateId].items[i].body[marker_pos];
        
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
                    if (automa[t].items[prod].production_id == kernelOfState[kernelProd].production_id && automa[t].items[prod].marker_position == kernelOfState[kernelProd].marker_position+1 && automa[t].items[prod].isKernelProduction){
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
int generateAutomaChar(struct automa_state* automa, struct lr0_item* grammar, int productions_count){
    int totalStates = 0; 

    // le seguenti due variabili servono per tenere traccia del numero di nuovi stati aggiunti a partire da ogni stato :
    // in particolare serviranno per evitare di creare un nuovo stato se esistesse gia
    char alreadyAddedNextChar[MAX_PRODUCTION_BODY_LENGTH];
    int addedStates = 0;

    //////////////// INIZIALIZZAZIONE /////////////////
    // aggiunta all'automa dello stato 0 con il suo kernel
    struct automa_state state0 = {
        .marked = false,
        .items_count = 0,
        .kernel_items_count = 1, // lo stato 0 contiene il kernel
        .transaction_count = 0
    };

    addProductionToClosure(&state0, grammar[0]); // aggiunta produzione inziiale al kernel dello stato iniziale
    automa[totalStates++] = state0;
    automa[totalStates-1].items[0].isKernelProduction = true;  // il primo item fa parte del kernel

    
    /////////////// SVOLGIMENTO //////////////////
    int unmarkedState = getUnmarkedStateId(automa, totalStates);
    while (unmarkedState != -1) {   // finche esiste uno stato unmarked
         
        // closure del kernel dello stato unmarked (uno stato unmarked contiene solo il kernel)       
        computeClosure(&automa[unmarkedState], grammar, productions_count);
        
        for (int i=0; i<automa[unmarkedState].items_count; i++){ // aggiunta delle transizioni a partire dagli stati unmarked

            // production e una produzione dello stato unmarked
            struct lr0_item production = automa[unmarkedState].items[i];

            int marker_pos = production.marker_position;
            if (marker_pos < strlen(production.body)){ // se il marker non è in ultima posizione
                bool alreadyAddedState = false;
                char nextChar = production.body[marker_pos];

                // duplicateStateOffset contiene l'offset dello stato uguale a partire dagli stati gia aggiunti per lo stato attuale preso in considerazione
                int duplicateStateOffset;
                // alreadyAddedStateId contiene l'indice dello stato convertito a partire dall'offset duplicateStateOffset
                int alreadyAddedStateId;
                for (duplicateStateOffset=0; duplicateStateOffset<addedStates; duplicateStateOffset++){
                    if (nextChar == alreadyAddedNextChar[duplicateStateOffset]){
                        alreadyAddedState = true;  // lo stato verso questo carattere e' gia stato inserito
                        break;
                    }
                }


                int kernelEqualTo = getKernelEqualTo(automa, totalStates, unmarkedState, nextChar);
                if (kernelEqualTo != -1){ // kernel gia presente, aggiungi solo la transizione verso lo stato specificato da kernelEqualTo
                    struct transaction newTransaction = {
                        .from = unmarkedState,
                        .by = production.body[marker_pos],  // il prossimo simbolo
                        .destination = kernelEqualTo
                    };

                    
                    printf("Tau (%d, %c) = %d \n", newTransaction.from, newTransaction.by, newTransaction.destination);
                    automa[unmarkedState].transactions[automa[unmarkedState].transaction_count++] = newTransaction;

                } else if (alreadyAddedState){ // se lo stato destinazione esiste gia allora aggiungo semplicemente la produzione al kernel 
                    alreadyAddedStateId = totalStates - addedStates + duplicateStateOffset;  
                    addProductionToKernel(&automa[alreadyAddedStateId], production); // aggiunta produzione al kernel dello stato gia essitente
                }else{
                    alreadyAddedNextChar[addedStates++] = production.body[marker_pos];

                    struct transaction newTransaction = {
                        .from = unmarkedState,
                        .by = production.body[marker_pos],  // il prossimo simbolo
                        .destination = totalStates
                    };

                    
                    printf("Tau (%d, %c) = %d \n", newTransaction.from, newTransaction.by, newTransaction.destination);
                    automa[unmarkedState].transactions[automa[unmarkedState].transaction_count++] = newTransaction;

                    // trovato nuovo stato, lo inizializziamo
                    struct automa_state newState = {
                        .marked = false,
                        .items_count = 0,
                        .kernel_items_count = 0,
                        .transaction_count = 0
                    };

                    addProductionToKernel(&newState, production); // aggiunta del kernel al nuovo stato
                    automa[totalStates++] = newState;

                }

                
            }else{ // marker in ultima posizione
                printf("reducing item\n");
            }

        }



        automa[unmarkedState].marked = true;
        unmarkedState = getUnmarkedStateId(automa, totalStates);
    
        addedStates = 0;  // reset del numero di nonTerminali gia aggiunti
    }

    return totalStates;

}


int main(int argc, char** argv){
    
    struct lr0_item grammar[MAX_GRAMMAR_PRODUCTIONS_NUMBER];
    char startSymbol;

    char production[MAX_PRODUCTION_BODY_LENGTH + 10];
    int productions_count = 0;
    int totalStates;

    if (argc != 2) {
        printf("Use %s <start_symbol>\n", argv[0]);
        exit(0);
    }
    startSymbol = argv[1][0];

    // estendi la grammatica P a P' : grammatica con aggiunta la produzione K -> startSymbol. K deve essere un fresh symbol.
    // Si controlla che K sia un fresh symbol utilizzando la funzione updateFreshSymbol dopo aver letto tutte le produzioni possibili.
    char fresh_production[] = "K -> _";
    fresh_production[5] = startSymbol;
    addProduction(grammar, fresh_production, &productions_count);

    // leggo le produzioni una ad una
    while (fgets(production, MAX_PRODUCTION_BODY_LENGTH + 10, stdin) && production[0] != '\n'){
        // rimuovo il carattere newline e gli spazi
        production[strlen(production) - 1] = '\0';  

        if (addProduction(grammar, production, &productions_count) == false){ 
            printf("La produzione non e' stata inserita in quanto non rispetta lo standard: A -> beta\n");
        }

    }

    updateFreshSymbol(grammar, productions_count);

    /////////////////////// STAMPA PRODUZIONI LETTE ///////////////////////
    printf("=================================  %d produzioni \n", productions_count);
    for (int i=0; i<productions_count; i++){
        printf("%c -> %s\n", grammar[i].driver, grammar[i].body);
    }

    ////////////////////////// CREAZIONE AUTOMA //////////////////////////

    printf("=================================\n");
    struct automa_state automa[50];
    totalStates = generateAutomaChar(automa, grammar, productions_count);


    /////////////////////////////// STAMPA //////////////////////////////
    for(int state=0; state<totalStates; state++){
        printf("============== STATO %d ============\n", state);
        for(int productionId = 0; productionId < automa[state].items_count; productionId++){
            struct lr0_item* production = &automa[state].items[productionId];
            printf("%c -> ", production->driver);
            for (int t=0; t<strlen(production->body); t++){
                if (t == production->marker_position){
                    printf(".%c",production->body[t]);
                }else{
                    printf("%c",production->body[t]);
                }
            }
            if (production->marker_position == strlen(production->body)){ // marker in ultima posizione
                printf(".");
            }
            
            printf("\n");
        }
        
    }
            
    return(0);
}
