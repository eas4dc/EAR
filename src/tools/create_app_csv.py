import pandas as pd
import argparse

from os import path


parser = argparse.ArgumentParser(
        prog='create_app_csv',
        description='Create an app csv file from a loop csv file.')
parser.add_argument('loop_csv_file')
parser.add_argument('--app-csv-file')

args = parser.parse_args()

loop_fn = args.loop_csv_file
loop_df = (pd
           .read_csv(loop_fn, sep=';')
           )

if args.app_csv_file:
    app_df = (pd
              .read_csv(args.app_csv_file, sep=';'))
    app_df['expr'] = app_df.apply(
                    lambda row: f'(not (JOBID == {row.JOBID} and STEPID == {row.STEPID} and APPID == {row.APPID} and NODENAME == "{row.NODENAME}"))', axis=1)
    query = ' and '.join(app_df['expr'].values)
    loop_df.query(query, inplace=True)

# Get all columns which are expected to be in an apps csv header
app_columns = (r'DCGMI_EVENTS_COUNT|GPU\d_gr_engine_active|GPU\d_sm_active'
               r'|GPU\d_sm_occupancy|GPU\d_tensor_active|GPU\d_dram_active'
               r'|GPU\d_fp64_active|GPU\d_fp32_active|GPU\d_fp16_active'
               r'|GPU\d_pcie_tx_bytes|GPU\d_pcie_rx_bytes|GPU\d_nvlink_tx_bytes'
               r'|GPU\d_nvlink_rx_bytes|COMP_BOUND|MPI_BOUND|IO_BOUND'
               r'|CPU_BUSY_WAITING|CPU_GPU_COMP|CPU_BOUND|MEM_BOUND|MIX_COMP'
               r'|NODENAME|JOBID|STEPID|APPID|USERID|GROUPID|JOBNAME|USER_ACC'
               r'|ENERGY_TAG|POLICY|POLICY_TH|START_TIME|END_TIME|START_DATE'
               r'|END_DATE|AVG_CPUFREQ_KHZ|AVG_IMCFREQ_KHZ|DEF_FREQ_KHZ|TIME_SEC'
               r'|CPI|TPI|MEM_GBS|IO_MBS|PERC_MPI|DC_NODE_POWER_W|DRAM_POWER_W'
               r'|PCK_POWER_W|CYCLES|INSTRUCTIONS|CPU-GFLOPS|L1_MISSES|L2_MISSES'
               r'|L3_MISSES|SPOPS_SINGLE|SPOPS_128|SPOPS_256|SPOPS_512|DPOPS_SINGLE'
               r'|DPOPS_128|DP_256|DPOPS_512|TEMP\d|GPU\d_POWER_W|GPU\d_FREQ_KHZ'
               r'|GPU\d_FREQ_KHZ|GPU\d_MEM_FREQ_KHZ|GPU\d_UTIL_PERC'
               r'|GPU\d_MEM_UTIL_PERC|GPU\d_GFLOPS|GPU\d_TEMP|GPU\d_MEMTEMP')
loop_df_filtered = loop_df.filter(regex=app_columns)

non_metric_cols = ['JOBID', 'STEPID', 'APPID', 'NODENAME']
metric_cols = loop_df_filtered.drop(columns=non_metric_cols)

cols_to_agg = ['L1_MISSES', 'L2_MISSES', 'L3_MISSES', 'SPOPS_SINGLE', 'SPOPS_128', 'SPOPS_256', 'SPOPS_512', 'DPOPS_SINGLE', 'DPOPS_128', 'DPOPS_256', 'DPOPS_512']
cols_aggfunc_dict = {col : ('sum' if col in cols_to_agg else 'mean') for col in metric_cols}

new_apps_df = (loop_df_filtered
               .pivot_table(index=['JOBID', 'STEPID', 'APPID', 'NODENAME'],
                            aggfunc=cols_aggfunc_dict)
               )
# print(new_apps_df)

loop_df_st = (loop_df
              .groupby(['JOBID', 'STEPID', 'APPID', 'NODENAME'])[['TIMESTAMP', 'ELAPSED']].min()
              .assign(
                  START_TIME=lambda df: df.TIMESTAMP - df.ELAPSED,
                  START_DATE=lambda df: pd.to_datetime(df.START_TIME, unit='s')  # Warning: Dates given in GMT + 0
                  )[['START_TIME', 'START_DATE']])
# print(loop_df_st)

loop_df_et = (loop_df
              .groupby(['JOBID', 'STEPID', 'APPID', 'NODENAME'])[['TIMESTAMP']].max()
              .rename(columns={'TIMESTAMP': 'END_TIME'})
              .assign(
                  END_DATE=lambda df: pd.to_datetime(df.END_TIME, unit='s')))  # Warning: Dates given in GMT + 0
# print(loop_df_et)

print(new_apps_df
      .join([loop_df_st, loop_df_et])
      .reset_index())

head_path, tail_path = path.split(loop_fn)
new_apps_fn = path.join(head_path, '_'.join(['apps', tail_path]))
with open(new_apps_fn, 'w') as f:
    (new_apps_df
     .join([loop_df_st, loop_df_et])
     .reset_index()
     .to_csv(f, sep=';', index=False))
