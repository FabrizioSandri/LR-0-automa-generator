struct lr0_item {
    char driver;
    char body[PRODUCTION_BODY_LENGTH];

    int marker_position;
    int production_id; 
    bool isKernelProduction;  
};
