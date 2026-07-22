<h1 align=center>
  📡 FlyRadar
</h1>
<h6 align=center>
  un mini radar open source pour voir les avions qui passent au dessus de votre tête 
</h6>
<p align=center>
  <img src="https://github.com/user-attachments/assets/2ccb2063-d15c-4180-8e3c-ae3a81c814ff" alt="drawing" width="400"/>
</p>
<p align=center>
  <a href="#Matériel">MATERIEL</a> - <a href="#assembly">ASSEMBLY</a> - <a href="#usage">USAGE</a> - <a href="#faq">FAQ</a>
</p>

Ce projet est un fork du projet micro-radar réalisé par AnthonuSturdy et consultable sur https://github.com/AnthonySturdy/micro-radar . J'avais envi de le réaliser mais avec le matériel à ma disposition dans mes tiroirs. Et j'ai ajouté quelques options de personalisation ainsi qu'une possibilité de mise à jour du firmware par OTA.

## Matériel

Le projet est construit autours d'un ESP32-C3 super mini et d'un écran LCD de 1,28' GC9A01 . il y aura quelques fils à souder entre les deux sinon il faut utiliser des cables dupont.

Pour le boitier j'ai utilisé mon imprimante 3D et du PLA (à vous de choisire la couleur..).  L'écran et l'esp sont fixés à l'aide de colle chaude (pistolet à colle). J'ai gardé 4 vis BTR M3x15 pour fixer le panneau écran mais c'est plus pour la déco, la colle aurait suffit. 

### Outils nécessaires

- clé allen 2,5  (pour les 4 vis)
- pistolet à colle
- Fer à a souder (si vous choississez l'option soudure....)

Attention l'écran est fragile et sensible aux rayures.

### Shopping List

Les composants sont facilement trouvables en ligne (amazon / aliexpress ) 

- [1.28" Round GC9A01  IPS Display ]
- [ESP32-C3 surper mini ]
- [4 vis M3x15]

pour l'esp, n'importequelle ESP32 fera l'affaire, mais il faudra adapter le cablage. 

### Accounts / API

Ce projet utilise l'API d'OpenSky pour récupérer les données de vol.
Je vous recommande vivement de créer un compte. C'est gratuit et cela permet au radar d'effectuer beaucoup plus de requêtes par jour (de 400 à 4 000), ce qui améliore considérablement la précision de l'affichage en temps réel. Cela reste toutefois facultatif si vous préférez ne pas en créer un.
Vous pouvez vous inscrire sur le site d'OpenSky  [here](https://opensky-network.org) (ou en recherchant simplement « OpenSky » sur votre moteur de recherche).
Vous trouverez davantage d'informations sur la configuration et l'utilisation du compte dans la section Utilisation de la documentation.

## Assembly

à venir

## Usage

### Flashing the Firmware

You'll need [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) installed. Once installed, restart VS Code, open the repository folder, and dependencies will pull in automatically.

Plug the board in via USB-C, then hit the upload button (→) in the bottom status bar. If the board doesn't reboot with the new firmware automatically, hold the BOOT button on the back and press RESET once, then release BOOT.

The board should auto-detect, but if you hit an upload failure, check that the correct board is selected in the status bar. If it still won't upload, try:

- Disconnect and reconnect the USB cable
- Check that your cable supports data transfer (some USB-C cables are charge-only)
- Try a different USB port on your computer

Read more about PlatformIO [here](https://docs.platformio.org/en/latest/).

### First Boot

On first boot, the radar broadcasts a WiFi hotspot called `FlyRadar-Setup`. Connect to it from your phone or laptop and a configuration page will appear automatically (or go to your browser if it doesn't). Enter your WiFi credentials and hit save. The board will restart and connect to your network.

If the hotspot doesn't appear straight away, give it a moment. If it still hasn't appeared after 30 seconds, exit the WiFi settings on your device and go back in to force a refresh. It'll usually show up then.

### Configuration

Once connected to your network, the radar config is accessible at [http://flyradar.local](http://flyradar.local) from any device on the same network.

Here you can set:

- **Location** (latitude and longitude): the centre point of your radar
- **Radar radius**: how wide the scan extends (in degrees, 2 degrees is the limit to avoid rate limiting)
- **Display options**: toggle visual elements
- **OpenSky credentials**: your client ID and secret (if you've made an account - again, highly recommend!)

<img width="400" alt="image" src="https://github.com/user-attachments/assets/45e6219c-2672-4197-baad-16ae08180b58" />

If you've made an OpenSky account (which I highly recommend), you can find your credentials under your account settings at opensky-network.org. With authentication, you get 4000 requests per day instead of 400, making the live view much more accurate. Read more about the API [here](https://opensky-network.org).

This configuration page is accessible anytime the device is connected to WiFi, so you can tweak settings whenever you want.

That's it! Once you've configured everything, you should see a live view of all flights over your location. Enjoy :)

<img width="400" alt="IMG_7935" src="https://github.com/user-attachments/assets/118b9a1c-c2c0-488d-b638-d8684a30b1d7" />

## FAQ

> the port is busy or doesn't exist

Restart VS Code *after* plugging in the device. If VS Code was already open, it may default to a stale port from before the device was connected.

If that doesn't work, look for the button with a small "Plug" icon on VS Code's bottom bar (it might say "auto", "cu.usbmodem101", or similar). Click it and select the option that shows your device's name.
<br/><br/>

> the 3D print failed

If you're using a Bambu Lab printer, make sure you're opening the `.3mf` file, since it includes the correct print bed and settings.

Using a different printer? Open an [Issue](../../issues) and I'll try to help where I can.
<br/><br/>

> `ModuleNotFoundError: No module named 'intelhex'` when building

This appears to be a Windows-specific issue. Either of these should fix it:

**Option A:**
1. Open the PlatformIO terminal (PlatformIO sidebar → Miscellaneous → PlatformIO Core CLI)
2. Run `pip install intelhex`
3. Rebuild

**Option B:**
1. Open a new terminal in VS Code (Terminal → New Terminal)
2. Run `python -m pip install intelhex`
3. Rebuild

## Notes

> Designed and developed as part of a wedding present for a mate who loves aviation (congratulations to both him and his wife!)

> Inspired by [therealhacksaw](https://www.instagram.com/therealhacksaw/)'s desk radar

> Built with ♥︎ in London
