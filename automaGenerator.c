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