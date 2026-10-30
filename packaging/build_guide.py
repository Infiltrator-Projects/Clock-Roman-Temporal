#!/usr/bin/env python3
"""Generate the clock provider's illustrated public PDF. No developer files installed."""
import json, math, sys
from pathlib import Path
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor
from reportlab.lib.pagesizes import A4
from reportlab.lib.utils import simpleSplit
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

ROOT=Path(__file__).resolve().parents[1]
W,H=A4
INK=HexColor("#21344A"); ACCENT=HexColor("#AE6038")
PALE=HexColor("#F0EDE5"); WHITE=HexColor("#FFFFFF")
MUTED=HexColor("#66798B"); PAPER=HexColor("#FBFAF6")
FONT,BOLD="Helvetica","Helvetica-Bold"
fontbase=Path("/usr/share/fonts/truetype/dejavu")
if (fontbase/"DejaVuSans.ttf").exists():
    pdfmetrics.registerFont(TTFont("Guide",str(fontbase/"DejaVuSans.ttf")))
    pdfmetrics.registerFont(TTFont("GuideBold",str(fontbase/"DejaVuSans-Bold.ttf")))
    FONT,BOLD="Guide","GuideBold"

# Each provider has its own plain-language explanation and visual scheme.
IDEAS={
"babylonian-ancient":("Twelve sunset-to-sunset bēru","The reconstructed day contains twelve bēru, each of thirty UŠ. The start of the count is computed sunset.",12,"sun"),
"babylonian-hours":("Hours counted from sunrise","The full day has 24 equal hours, counted from the local computed sunrise. These are Renaissance European Babylonian hours.",24,"sun"),
"binary":("A clock written in zeroes and ones","Binary uses 0 and 1 to write numerical quantities. For example, decimal thirteen is binary 1101. It changes the notation, not the length of the day.",16,"base"),
"chinese-ke":("One hundred equal divisions","The day is divided into 100 kè. A kè is one-hundredth of a 24-hour day: 14 minutes and 24 seconds.",100,"segments"),
"chinese-time":("Twelve traditional double-hours","The 24-hour day is divided into twelve successive double-hour segments named after the Earthly Branches.",12,"segments"),
"decimal":("A day with ten hours","Each day has ten decimal hours, each decimal hour has one hundred decimal minutes, and each decimal minute has one hundred decimal seconds.",10,"segments"),
"hexadecimal":("The day expressed in base sixteen","Hexadecimal uses the digits 0–9 and the letters A–F. Fifteen is written F; sixteen is written 10. It is a different numerical representation.",16,"base"),
"indian-ghati":("Sixty ghaṭī measured from sunrise","Sixty ghaṭī make a mean day, with pala subdivisions. One ghaṭī is 24 ordinary minutes; the count begins at computed sunrise.",60,"sun"),
"internet":("One thousand beats per day","Internet Time divides a fixed-reference day into one thousand beats. A beat lasts 86.4 seconds; the reference offset is UTC+01:00 (Biel).",1000,"segments"),
"italian-hours":("Hours counted from sunset","Twenty-four equal hours are counted beginning with the local computed sunset, so the starting point changes with season and place.",24,"sun"),
"japanese-temporal":("Seasonal bell hours in late Edo Japan","Daylight and night are each divided into six intervals of changing duration. The model uses a particular late-Edo dawn/dusk definition.",12,"sun"),
"japanese-temporal-early":("Seasonal bell hours in early Edo Japan","Daylight and night are each divided into six varying intervals; this early-Edo model uses a different dawn/dusk boundary.",12,"sun"),
"julian":("A continuous astronomical day number","Julian Date counts astronomical days as a number, with the conventional day boundary at noon UTC. It is not a local wall clock.",1,"number"),
"mean-solar":("Mean solar time from longitude","A theoretical evenly moving Sun sets the time scale. Locations separated by about fifteen degrees of longitude differ by one mean solar hour.",24,"sun"),
"modified-julian":("A smaller astronomical day number","Modified Julian Date is Julian Date minus 2,400,000.5. The modified day-number boundary is at midnight UTC.",1,"number"),
"nuremberg-hours":("The historical Nuremberg civic schedule","The Great Clock follows a fixed civic schedule of seasonal changeover days, rather than calculating each day's sunrise directly.",24,"segments"),
"nuremberg-solar":("A reconstructed seasonal Nuremberg clock","Computed local sunrise and sunset govern a seasonal division of hours. This is not the fixed historical civic changeover schedule.",24,"sun"),
"roman-temporal":("Twelve unequal daylight hours","The interval between sunrise and sunset is divided into twelve daylight hours. These are longer in summer and shorter in winter; nights use watches.",12,"sun"),
"sidereal":("Time by the turning stars","Sidereal time follows the apparent rotation of distant stars. A sidereal day lasts about 23 hours and 56 minutes of ordinary solar time.",24,"sun"),
"solar":("Time measured by the actual Sun","Apparent solar time follows the Sun's changing position. Apparent noon is when the Sun crosses the local meridian.",24,"sun"),
"standard-12":("The familiar morning/afternoon clock","A day uses two twelve-hour cycles distinguished by AM and PM. For example, 15:30 is 3:30 PM.",12,"dial"),
"standard-24":("The familiar 24-hour civil clock","A full civil day runs from 00:00 to 23:59. For example, 3:30 PM is written 15:30.",24,"dial"),
"unix":("Counting seconds from 1970","Unix time expresses an instant as elapsed seconds from 1 January 1970, 00:00 UTC. It is a time coordinate, not a clock face.",1,"number")
}
def text(c,s,x,y,sz=10,b=False,col=INK):
    c.setFillColor(col);c.setFont(BOLD if b else FONT,sz);c.drawString(x,y,str(s))
def wrap(c,s,x,y,width,sz=10,leading=18):
    for line in simpleSplit(str(s),FONT,sz,width):
        text(c,line,x,y,sz);y-=leading
    return y
def page(c,n,heading):
    c.setFillColor(PAPER);c.rect(0,0,W,H,fill=1,stroke=0)
    text(c,"INFILTRATOR  /  CLOCKS",45,H-34,9,True,MUTED)
    text(c,heading,45,H-94,20,True)
    c.setStrokeColor(PALE);c.line(45,48,W-45,48)
    text(c,"ILLUSTRATED GUIDE",45,30,8,False,MUTED)
    text(c,f"{n} / 3",W-78,30,8,False,MUTED)
def drawing(c,kind,n,cx,cy):
    if kind=="base":
        text(c,"0  1  0  1   1  0  1  0",cx-198,cy+12,24,True,ACCENT)
        text(c,"BITS AND DIGITS  /  AN ILLUSTRATION",cx-188,cy-23,9,True,MUTED)
    elif kind=="number":
        c.setFillColor(PALE);c.roundRect(cx-223,cy-50,446,106,9,fill=1,stroke=0)
        c.setStrokeColor(ACCENT);c.setLineWidth(3);c.line(cx-170,cy,cx+170,cy)
        for k in range(5):
            xx=cx-168+84*k
            c.setFillColor(INK);c.circle(xx,cy,5,stroke=0,fill=1)
        text(c,"EARLIER",cx-195,cy+26,8,True,MUTED)
        text(c,"LATER",cx+150,cy+26,8,True,MUTED)
        text(c,"CONTINUOUS NUMERIC REFERENCE",cx-134,cy-28,8,True,ACCENT)
    elif kind=="segments":
        count=min(n,20);left=cx-216;width=432/count
        for k in range(count):
            c.setFillColor(ACCENT if k%2 else INK)
            c.roundRect(left+k*width,cy-22,max(2,width-2),50,3,stroke=0,fill=1)
        text(c,f"{n} PARTS OF THE REFERENCE DAY",left,cy-56,10,True,ACCENT)
        if count<n:text(c,"Diagram groups divisions; not every division is drawn.",left,cy-77,9,False,MUTED)
    else:
        c.setStrokeColor(ACCENT);c.setLineWidth(7);c.circle(cx,cy,82,fill=0,stroke=1)
        c.setStrokeColor(INK);c.setLineWidth(1.5)
        for k in range(min(n,24)):
            a=math.pi/2-(2*math.pi*k/min(n,24))
            c.line(cx+67*math.cos(a),cy+67*math.sin(a),cx+77*math.cos(a),cy+77*math.sin(a))
        c.setLineWidth(5);c.line(cx,cy,cx+38,cy+43)
        c.setFillColor(ACCENT);c.circle(cx,cy,5,fill=1,stroke=0)
        text(c,"SOLAR / SEASONAL MODEL" if kind=="sun" else "CIVIL CLOCK",cx-76,cy-119,9,True,MUTED)
def main():
    manifest=json.loads((ROOT/"SOURCE-MANIFEST.json").read_text())
    key=manifest["id"];name=manifest["name"]
    if key not in IDEAS:raise ValueError("Clock guide missing reviewed provider: "+key)
    heading,how,n,kind=IDEAS[key]
    readme=(ROOT/"README.md").read_text(encoding="utf-8")
    lines=readme.splitlines()
    intro=next((s.strip() for s in lines[3:12] if s.strip() and not s.startswith("**")),None)
    scope=next((s.split("**Historical scope/accuracy:**",1)[1].strip() for s in lines if "**Historical scope/accuracy:**" in s),None)
    if not intro or not scope or ("# "+name) not in readme:
        raise ValueError("Model identity and accuracy boundaries must be in README")
    geographic="needs valid geographic coordinates" in readme or "needs geographic coordinates" in readme
    output=Path(sys.argv[1]);output.parent.mkdir(parents=True,exist_ok=True)
    c=canvas.Canvas(str(output),pagesize=A4,invariant=1)
    c.setTitle(name+" — Illustrated clock guide");c.setAuthor("Infiltrator Projects")
    page(c,1,"WHAT IS THIS CLOCK?")
    y=H-147
    for line in simpleSplit(name,BOLD,17,W-90):text(c,line,45,y,17,True,ACCENT);y-=26
    y=wrap(c,intro,45,y-17,W-90,11,19)-38
    c.setFillColor(PALE);c.roundRect(45,y-118,W-90,127,9,fill=1,stroke=0)
    text(c,"THE ESSENTIAL IDEA",62,y-20,10,True,ACCENT)
    wrap(c,how,62,y-48,W-124,10,17)
    drawing(c,kind,n,W/2,250)
    text(c,"The illustration explains the system; it is not a live time reading.",45,92,9,False,MUTED)
    c.showPage()
    page(c,2,"HOW TO READ IT")
    text(c,heading.upper(),45,H-149,12,True,ACCENT)
    wrap(c,how,45,H-181,W-90,11,19)
    drawing(c,kind,n,W/2,H-396)
    text(c,"WHAT CHANGES COMPARED WITH A NORMAL CLOCK?",45,207,10,True,ACCENT)
    wrap(c,intro,45,181,W-90,10,19)
    text(c,"A visual explanation, not a substitute for the live provider.",45,92,9,False,MUTED)
    c.showPage()
    page(c,3,"HISTORY, ACCURACY AND USE")
    text(c,"HISTORICAL CONTEXT",45,H-149,11,True,ACCENT)
    y=wrap(c,scope,45,H-179,W-90,11,20)-43
    text(c,"WHAT THE IMPLEMENTATION DOES",45,y,11,True,ACCENT)
    y=wrap(c,intro,45,y-27,W-90,10.5,19)-31
    rule=("This model needs location information. If the Sun cannot be located for the selected position and date, an output may be unavailable."
          if geographic else "This model does not require location coordinates. Its time reference may nevertheless differ from your local civil wall time.")
    y=wrap(c,rule,45,y,W-90,10,19)-32
    text(c,"THE IMPORTANT LIMIT",45,y,11,True,ACCENT)
    wrap(c,"The computer implements the specific model described here. It should not be treated as a claim about every historical locality, instrument, reform or astronomical observation.",45,y-26,W-90,10,18)
    c.setFillColor(PALE);c.roundRect(45,89,W-90,69,8,fill=1,stroke=0)
    text(c,"DOCUMENTATION AND SOURCES",60,139,8,True,ACCENT)
    wrap(c,"github.com/Infiltrator-Projects/"+manifest["repository"],60,112,W-115,8,13)
    c.save()
if __name__=="__main__":main()
