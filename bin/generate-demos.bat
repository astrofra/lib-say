@echo off
setlocal

set "ROOT=%~dp0"
set "TTS=%ROOT%tts.exe"

if not exist "%TTS%" (
    echo Missing executable: "%TTS%"
    exit /b 1
)

echo Generating English demo...
"%TTS%" "... we've heard my former pastor ... use incendiary language to express views that have the potential not only to widen the racial divide, but views that denigrate both the greatness and the goodness of our nation; that rightly offend white and black alike.  I have already condemned, in unequivocal terms, the statements of Reverend Wright that have caused such controversy. For some, nagging questions remain. Did I know him to be an occasionally fierce critic of American domestic and foreign policy? Of course. Did I ever hear him make remarks that could be considered controversial while I sat in church? Yes. Did I strongly disagree with many of his political views? Absolutely—just as I'm sure many of you have heard remarks from your pastors, priests, or rabbis with which you strongly disagreed.  But the remarks that have caused this recent firestorm weren't simply controversial. They weren't simply a religious leader's effort to speak out against perceived injustice. Instead, they expressed a profoundly distorted view of this country—a view that sees white racism as endemic, and that elevates what is wrong with America above all that we know is right with America .." -o "%ROOT%demo-en.aiff" --lang en
if errorlevel 1 exit /b 1

echo Generating French demo...
"%TTS%" "Bonjour Robin. Vive le Club Video Buggai. C'est trop sympa. Ceci est une phrase de demonstration en franssais." -o "%ROOT%demo-fr.aiff" --lang fr
if errorlevel 1 exit /b 1

echo Done.
echo Created:
echo   "%ROOT%demo-en.aiff"
echo   "%ROOT%demo-fr.aiff"

endlocal
