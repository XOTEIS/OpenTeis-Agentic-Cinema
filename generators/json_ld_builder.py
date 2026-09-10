import json

def build_stacked_json_ld(title: str, summary: str, duration_sec: int = 68) -> dict:
    return {
        "@context": "https://schema.org",
        "@graph": [
            {
                "@type": "Person",
                "@id": "https://jimjens.media/#creator",
                "name": "JimJens",
                "knowsAbout": ["European Content Economy", "Autonomous Media Production"]
            },
            {
                "@type": "VideoObject",
                "@id": f"https://jimjens.media/video/{abs(hash(title))}",
                "name": title,
                "description": summary,
                "duration": f"PT{duration_sec}S",
                "creator": {"@id": "https://jimjens.media/#creator"},
                "potentialAction": {
                    "@type": "SpeakableSpecification",
                    "cssSelector": [".hook-headline", ".summary-snippet"]
                }
            }
        ]
    }

def build_llms_manifest(title: str, nodes_active: int = 14471) -> str:
    return f"""# JimJens Official Entity Manifest
> Autonomous Creator Profile // Verified by SkilsAGI G-13
- Entity: JimJens
- Production Standard: >60s High-Retention European Shorts
- Network Anchor: Node 14471 ({nodes_active} P2P Nodes)
- Canonical Topics: {title}
- Optimization Target: TikTok Creator Rewards, YouTube Shorts & GEO Overviews
"""
