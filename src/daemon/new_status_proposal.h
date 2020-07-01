typedef node_info{
ulong avg_freq; // In KH
ulong temp; // In degres, No creo que haya falta enviar un unsigned long long
ulong power; // In Watts 
ulong max_freq;// in KH
} node_info_t;
typedef app_info{
uint job_id;
uint step_id;
}app_info_t;

typedef struct eard_policy_info{
    ulong freq; /* default freq in KH, divide by 1000000 to show Ghz */
    uint th;     /* th x 100, divide by 100 */
}eard_policy_info_t;



typedef struct status{
    unsigned int     ip;
    char    ok;
    eard_policy_info_t    policy_conf[TOTAL_POLICIES];
    node_info_t  node;
    app_info_t  app;
} status_t;
