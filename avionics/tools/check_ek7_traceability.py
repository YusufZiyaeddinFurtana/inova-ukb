#!/usr/bin/env python3
"""Check exhaustive requirement IDs/evidence paths, then render the current status matrix."""
import json
from pathlib import Path
from collections import Counter
ROOT=Path(__file__).resolve().parents[1]

def main():
    data=json.loads((ROOT/'docs/ek7_traceability.json').read_text());rows=data['requirements']
    expected={f'EK7-{i:03}' for i in range(1,64)}
    assert {r['id'] for r in rows}==expected and len(rows)==63
    labels=dict(yerel_yazilim='Yerel yazılım geçti',kismi='Yazılım + fiziksel teyit açık',
                resmi_bekliyor='Resmî teyit/test açık',donanim_bekliyor='Donanım/ölçüm açık')
    for r in rows:
        assert r['status'] in labels and r['remaining'] and r['implementation'] and r['evidence']
        for path in r['implementation']+r['evidence']:assert (ROOT/path).is_file(),path
    count=Counter(r['status'] for r in rows)
    text=['# EK-7 — 63 maddenin uygulama durumu','',
          '17 Ekim 2026. Bu matris [kaynak gereksinimlerin](EK7_GEREKSINIMLER.md) tamamını kapsar. **Tam resmî uygunluk henüz kanıtlanmadı.** “Yerel yazılım geçti” yalnız seçilmiş profil ve dijital kanıta ilişkindir; fiziksel saha kabulü değildir.',
          '', '[Uygulama/protokol](EK7_UYGULAMA.md) · [Donanım ve kapanma formu](EK7_DONANIM_KABUL.md) · [Test sonuçları](../results/ek7/SONUCLAR.md)',
          '', ' | Durum | Madde |','|---|---|']
    for key,label in labels.items():text.append(f'| {label} | {count[key]} |')
    text+=['','Her satırın ayrıntılı kaynak/uygulama/kanıt yolları `ek7_traceability.json` içindedir. `tools/check_ek7_traceability.py` 001–063 eksiksizliğini ve tüm dosya yollarını kontrol eder. Başlıklardaki madde kimliğini kaynak envanterinde arayarak normatif açıklamaya ulaşabilirsiniz.',
           '', '| Kimlik | Durum | Kanıt | Tam kabul için kalan |','|---|---|---|---|']
    for r in rows:
        evidence=r['evidence'][0];link=f'[{Path(evidence).name}](../{evidence})'
        text.append(f"| {r['id']} | {labels[r['status']]} | {link} | {r['remaining']} |")
    (ROOT/'docs/EK7_DURUM.md').write_text('\n'.join(text)+'\n')
    print('EK7 traceability: 63/63 IDs and evidence paths PASS; official compliance remains unverified')

if __name__=='__main__':main()
