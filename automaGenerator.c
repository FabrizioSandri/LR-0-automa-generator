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

    for (bodyStartPosition=1; bodyStartPosition<strlen(production); bodyStartPosition++){
        if (production[bodyStartPosition-1] == '-' && production[bodyStartPosition] == '>' ){
            bodyStartPosition++;
            foundArrow = true;
            break;
        }
    }

    if (foundArrow && isNonTerminal(production[0])){
        // rimozione spazi vuoti dalla produzione
        while(production[bodyStartPosition] == ' ' && bodyStartPosition < strlen(production)){
            bodyStartPosition++;
        }

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

int main(int argc, char** argv){
    
    struct lr0_item grammar[MAX_GRAMMAR_PRODUCTIONS_NUMBER];
    char startSymbol;

    char production[MAX_PRODUCTION_BODY_LENGTH + 10];
    int productions_count = 0;
    
    if (argc != 2) {
        printf("Use %s <start_symbol>\n", argv[0]);
        exit(0);
    }
    startSymbol = argv[1][0];

    // estendi la grammatica P a P' : grammatica con aggiunta la produzione S -> startSymbol
    char fresh_production[] = "K -> _";
    fresh_production[5] = startSymbol;
    addProduction(grammar, fresh_production, &productions_count);

    // leggo le produzioni una ad una
    while (fgets(production, MAX_PRODUCTION_BODY_LENGTH + 10, stdin)[0] != '\n'){
        // rimuovo il carattere newline
        production[strlen(production) - 1] = '\0';  
        
        if (addProduction(grammar, production, &productions_count) == false){ 
            printf("La produzione non e' stata inserita in quanto non rispetta lo standard: A -> beta\n");
        }

    }


    printf("=====================  %d produzioni \n", productions_count);
    // DEBUG
    for (int i=0; i<productions_count; i++){
        printf("%c -> %s\n", grammar[i].driver, grammar[i].body);
    }

    return(0);
}