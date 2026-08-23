import pandas as pd
import numpy as np
from datetime import datetime, timedelta

class Planner:
    """
    BatterySwapAI 2026 Competition Planner
    """
    def __init__(self):
        pass

    def plan(self, timeseries: pd.DataFrame, locations: pd.DataFrame, travel_costs: pd.DataFrame, settings) -> pd.DataFrame:
        start_date = pd.to_datetime(getattr(settings, 'start_date', timeseries['end_time'].max())).date()
        window_days = getattr(settings, 'planning_window_days', 42)
        end_date = start_date + timedelta(days=window_days)
        no_swap_date = end_date + timedelta(days=7)

        loc_df = locations.copy()
        dev_col = 'device_id' if 'device_id' in loc_df.columns else loc_df.columns[0]
        building_col = 'building_id' if 'building_id' in loc_df.columns else loc_df.columns[1]
        room_col = 'room_id' if 'room_id' in loc_df.columns else loc_df.columns[2]

        loc_map = loc_df.set_index(dev_col)[[building_col, room_col]].to_dict('index')
        all_batteries = list(loc_map.keys())

        ts = timeseries.copy()
        ts['end_time'] = pd.to_datetime(ts['end_time'])
        
        device_rul = {}
        for b_id, g in ts.groupby('device_id' if 'device_id' in ts.columns else ts.columns[0]):
            g = g.sort_values('end_time')
            if len(g) >= 24:
                last_v = float(g['voltage'].iloc[-1])
                last_t = g['end_time'].iloc[-1].date()
                v_norm = g['voltage'] - 0.005 * (g['temperature'] - 20.0) if 'temperature' in g.columns else g['voltage']
                delta_days = (g['end_time'].iloc[-1] - g['end_time'].iloc[0]).total_seconds() / 86400.0
                if delta_days > 1.0:
                    slope = (v_norm.iloc[-1] - v_norm.iloc[0]) / delta_days
                    if slope < -0.0001:
                        days_to_eol = (last_v - 2.40) / abs(slope)
                        pred_eol = last_t + timedelta(days=max(0.0, float(days_to_eol)))
                    else:
                        pred_eol = last_t + timedelta(days=200)
                else:
                    pred_eol = last_t + timedelta(days=120)
            else:
                last_t = g['end_time'].iloc[-1].date() if len(g) > 0 else start_date
                pred_eol = last_t + timedelta(days=120)
            device_rul[b_id] = pred_eol

        swaps_in_window = []
        no_swaps = []

        for b_id in all_batteries:
            eol_date = device_rul.get(b_id, start_date + timedelta(days=120))
            loc = loc_map.get(b_id, {building_col: 'b_0', room_col: 'r_0'})
            if start_date <= eol_date <= end_date:
                swaps_in_window.append({
                    'battery': b_id,
                    'pred_eol': eol_date,
                    'building': loc[building_col],
                    'room': loc[room_col]
                })
            else:
                no_swaps.append({
                    'day': str(no_swap_date),
                    'battery': b_id
                })

        swaps_in_window.sort(key=lambda x: x['pred_eol'])
        if len(swaps_in_window) > 16:
            overflow = swaps_in_window[16:]
            swaps_in_window = swaps_in_window[:16]
            for item in overflow:
                no_swaps.append({
                    'day': str(no_swap_date),
                    'battery': item['battery']
                })

        swaps_in_window.sort(key=lambda x: (x['building'], x['room'], x['pred_eol']))

        scheduled_swaps = []
        curr_day = start_date + timedelta(days=2)
        daily_count = 0

        for item in swaps_in_window:
            while curr_day.weekday() >= 5:
                curr_day += timedelta(days=1)
                daily_count = 0
                
            scheduled_swaps.append({
                'day': str(curr_day),
                'battery': item['battery']
            })
            daily_count += 1
            if daily_count >= 4:
                curr_day += timedelta(days=1)
                daily_count = 0

        full_plan = pd.DataFrame(scheduled_swaps + no_swaps)
        return full_plan[['day', 'battery']]

# Fallback alias dersom runneren søker etter modellspesifikt navn
OpenTeisPlanner = Planner
