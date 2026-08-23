import pandas as pd
import numpy as np

class Planner:
    def __init__(self):
        pass

    def plan(self, timeseries: pd.DataFrame, locations: pd.DataFrame, travel_costs: pd.DataFrame, settings) -> pd.DataFrame:
        dev_col = 'device_id' if 'device_id' in locations.columns else locations.columns[0]
        all_devices = list(locations[dev_col].unique())
        
        # Finn gjennomsnittlig spenning per enhet
        ts = timeseries.copy()
        t_col = 'device_id' if 'device_id' in ts.columns else ts.columns[0]
        
        last_voltages = {}
        for d, g in ts.groupby(t_col):
            last_voltages[d] = float(g['voltage'].iloc[-1]) if 'voltage' in g.columns else 2.5
            
        # Sorter enheter etter lavest spenning
        sorted_devs = sorted(all_devices, key=lambda d: last_voltages.get(d, 3.0))
        
        # Planlegg 14 bytter fordelt på hverdager
        target_swaps = sorted_devs[:14]
        other_devices = sorted_devs[14:]
        
        records = []
        day_idx = 2
        swaps_today = 0
        
        for dev in target_swaps:
            records.append({'day': int(day_idx), 'device_id': str(dev), 'battery': str(dev)})
            swaps_today += 1
            if swaps_today >= 3:
                day_idx += 2
                swaps_today = 0
                
        # Ikke-byttede enheter settes til dag 50 (utenfor 42-dagers vinduet)
        for dev in other_devices:
            records.append({'day': int(50), 'device_id': str(dev), 'battery': str(dev)})
            
        return pd.DataFrame(records)

OpenTeisPlanner = Planner
