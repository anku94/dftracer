import pandas as pd
import numpy as np

def extract_bw_stats(filename):
    df = pd.read_csv(filename)
    stats = {}
    for access_type in ['write', 'read']:
        bw = df[df['access'] == access_type]['bw(MiB/s)']
        stats[access_type] = {
            'avg': np.mean(bw),
            'std': np.std(bw)
        }
    return stats

baseline_stats = extract_bw_stats('baseline.csv')
dftracer_stats = extract_bw_stats('dftracer.csv')

print("access_type,baseline_avg,baseline_std,dftracer_avg,dftracer_std,overhead_percent")
for access_type in ['write', 'read']:
    base_avg = baseline_stats[access_type]['avg']
    base_std = baseline_stats[access_type]['std']
    dftr_avg = dftracer_stats[access_type]['avg']
    dftr_std = dftracer_stats[access_type]['std']
    overhead = ((dftr_avg - base_avg) / base_avg) * 100 if base_avg != 0 else 0
    print(f"{access_type},{base_avg:.2f},{base_std:.2f},{dftr_avg:.2f},{dftr_std:.2f},{overhead:.2f}")