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
};


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
    
    return(0);
}