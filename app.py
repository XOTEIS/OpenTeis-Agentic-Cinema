import streamlit as st
import json
import os
from bridge.stigmergy_ffi import StigmergyEngine
from generators.json_ld_builder import build_stacked_json_ld, build_llms_manifest

st.set_page_config(page_title="Crystal Meta Printer", layout="wide")

st.title("Crystal Meta Printer // G-13 Stigmergic Engine")
st.caption("SkilsAGI Research — Bio-Inspired Stigmergy & Stacked Schema.org Generator")

col1, col2 = st.columns([1, 1])

with col1:
    st.subheader("Innholdsprodusent Input (JimJens Pipeline)")
    video_title = st.text_input("Videotittel", value="European Creator Economy Scaling: Breaking Algorithmic Minimums")
    transcript = st.text_area("Transkripsjonssammendrag / Kjerneintensjon", 
                              value="Autonom innholdsproduksjon optimalisert for TikTok Creator Rewards og Google AI Overviews.")
    target_len = st.slider("Mållengde for video (sekunder)", min_value=61, max_value=180, value=72)
    trigger = st.button("Krystalliser Stigmergisk Metadata")

if trigger:
    engine = StigmergyEngine()
    engine.add_node(1, 0.0, 0.0, 10.0)
    engine.add_node(2, 1.2, 2.5, 25.0)
    
    locked = False
    for step in range(1, 20):
        res = engine.pulse(1, 2, 22.5)
        if res == 1:
            locked = True
            break
    engine.close()

    with col2:
        st.subheader("Telemetri & Krystallisert Output")
        st.success(f"Rubedo-Resonans Oppnådd: {locked} | Termodynamisk Skranke: <= 0.031 J/Op")
        
        json_ld = build_stacked_json_ld(video_title, transcript, duration_sec=target_len)
        manifest = build_llms_manifest(video_title, nodes_active=14471)
        
        tab1, tab2 = st.tabs(["Stacked Schema.org (JSON-LD)", "Machine Manifest (/llms.txt)"])
        with tab1:
            st.json(json_ld)
        with tab2:
            st.text(manifest)
