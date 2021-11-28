struct automa_state {
    struct lr0_item items[50];  
    struct transition transitions[AUTOMA_STATES_COUNT];
    
    state_type type;

    int items_count;
    int kernel_items_count;
    int transition_count;
};
