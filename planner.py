import pandas as pd
import numpy as np
from datetime import datetime, timedelta

class Planner:
    def __init__(self):
        pass

    def plan(self, timeseries: pd.DataFrame, locations: pd.DataFrame, travel_costs: pd.DataFrame, settings) -> pd.DataFrame:
        # Hent startdato
        start_date = getattr(settings, 'start_date', None)
        if start_date is None:
            if 'end_time' in timeseries.columns:
                start_date = pd.to_datetime(timeseries['end_time'].max()).date()
            else:
                start_date = datetime.now().date()
        elif isinstance(start_date, str):
            start_date = pd.to_datetime(start_date).date()
        elif hasattr(start_date, 'date'):
            start_date = start_date.date()

        window_days = int(getattr(settings, 'planning_window_days', 42))
        end_date = start_date + timedelta(days=window_days)
        no_swap_date = end_date + timedelta(days=7)

        # Identifiser kolonner i locations
        dev_col = 'device_id' if 'device_id' in locations.columns else locations.columns[0]
        building_col = 'building_id' if 'building_id' in locations.columns else (locations.columns[1] if len(locations.columns) > 1 else dev_col)
        room_col = 'room_id' if 'room_id' in locations.columns else (locations.columns[2] if len(locations.columns) > 2 else dev_col)

        all_devices = list(locations[dev_col].unique())
        loc_map = {}
        for _, row in locations.iterrows():
            loc_map[row[dev_col]] = {
                'building': str(row[building_col]) if building_col in row else 'b0',
                'room': str(row[room_col]) if room_col in row else 'r0'
            }

        # Behandle tidsserier og spenning
        ts = timeseries.copy()
        t_dev_col = 'device_id' if 'device_id' in ts.columns else ts.columns[0]
        if 'end_time' in ts.columns:
            ts['end_time'] = pd.to_datetime(ts['end_time'])
        
        last_voltages = {}
        for d, g in ts.groupby(t_dev_col):
            if 'voltage' in g.columns and len(g) > 0:
                last_voltages[d] = float(g['voltage'].iloc[-1])
            else:
                last_voltages[d] = 2.5

        # Sorter enheter etter spenningsfall (lavest først)
        sorted_devs = sorted(all_devices, key=lambda d: last_voltages.get(d, 3.0))

        # Velg ut de 14 mest kritiske enhetene
        target_swaps = sorted_devs[:14]
        other_devices = sorted_devs[14:]

        # Klyngesorter de utvalgte etter bygning og rom
        target_swaps.sort(key=lambda d: (loc_map.get(d, {}).get('building', 'b0'), loc_map.get(d, {}).get('room', 'r0')))

        records = []
        curr_day = start_date + timedelta(days=2)
        daily_count = 0

        for dev in target_swaps:
            while curr_day.weekday() >= 5:  # Hopp over helg
                curr_day += timedelta(days=1)
                daily_count = 0
                
            records.append({
                'day': curr_day,
                'device_id': str(dev),
                'battery': str(dev),
                'battery_id': str(dev)
            })
            daily_count += 1
            if daily_count >= 3:
                curr_day += timedelta(days=1)
                daily_count = 0

        # Resterende sensorer settes til etter planleggingsvinduet
        for dev in other_devices:
            records.append({
                'day': no_swap_date,
                'device_id': str(dev),
                'battery': str(dev),
                'battery_id': str(dev)
            })

        df = pd.DataFrame(records)
        return df

OpenTeisPlanner = Planner
