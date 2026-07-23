<h1 align=center>
  📡 FlyRadar
</h1>
<h6 align=center>
</h6>
<p align=center>
  <img src="/docs/images/IMG_5903.gif" alt="drawing" width="400"/>
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

Ce projet est un fork du projet micro-radar réalisé par AnthonySturdy et consultable sur https://github.com/AnthonySturdy/micro-radar. J'avais envie de le réaliser avec le matériel disponible dans mes tiroirs. J'ai ajouté quelques options de personnalisation et la possibilité de mise à jour du firmware par OTA.

<img src="/docs/images/IMG_5898.jpeg" alt="drawing" width="400"/>
il faut souder ou relier via des cables dupont les pins dans l'ordre suivant: 
Le projet est construit autour d'un ESP32-C3 (super-mini) et d'un écran LCD 1,28" GC9A01. Il faudra quelques fils à souder ou utiliser des câbles Dupont.
3V3 // VCC
GND // GND
GPIO4 // SCL
GPIO5 // SDA
GPIO9 // DC
  <img src="docs/images/esp32.png" width="45%" />
  <img src="docs/images/IMG_5899.jpeg" width="45%" />
</p>

L'assemblage est relativement simple. J'ai utilisé un pistolet à colle chaude. L'écran est collé dans la pièce 'porte écran'. L'esp 32 est également collé au fond de la base: colle en dessous puis un point sur le dessus à l'opposé du port usb.
- [1.28" Round GC9A01 IPS Display]
- [ESP32-C3 super-mini]
- [4 vis M3x15]
<p align=center>
Pour l'ESP, n'importe quelle carte ESP32 peut convenir, mais il faudra adapter le câblage.



Ce projet utilise l'API d'OpenSky pour récupérer les données de vol. Je vous recommande vivement de créer un compte : c'est gratuit et cela permet au radar d'effectuer beaucoup plus de requêtes par jour (environ 4 000 au lieu de 400), ce qui améliore la précision du flux en direct. Cela reste facultatif.
Inscrivez‑vous sur https://opensky-network.org. Vous trouverez plus d'informations dans la section « Utilisation ».

You'll need [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) installed. Once installed, restart VS Code, open the repository folder, and dependencies will pull in automatically.

Plug the board in via USB-C, then hit the upload button (→) in the bottom status bar. If the board doesn't reboot with the new firmware automatically, hold the BOOT button on the back and press RESET once, then release BOOT.
Soudez ou reliez (Dupont) les broches comme suit :
ESP32-C3 // LCD 1,28" GC9A01
<p align="center">
  <img src="docs/images/esp32.png" width="45%" />
  <img src="docs/images/IMG_5899.jpeg" width="45%" />
</p>

### First Boot

On first boot, the radar broadcasts a WiFi hotspot called `FlyRadar-Setup`. Connect to it from your phone or laptop and a configuration page will appear automatically (or go to your browser if it doesn't). Enter your WiFi credentials and hit save. The board will restart and connect to your network.

If the hotspot doesn't appear straight away, give it a moment. If it still hasn't appeared after 30 seconds, exit the WiFi settings on your device and go back in to force a refresh. It'll usually show up then.

### Configuration

Once connected to your network, the radar config is accessible at [http://flyradar.local](http://flyradar.local) from any device on the same network.
Installez [VS Code](https://code.visualstudio.com/) avec l'extension [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide). Ouvrez le projet, PlatformIO installera les dépendances automatiquement.
Here you can set:

- **Location** (latitude and longitude): the centre point of your radar
- **Radar radius**: how wide the scan extends (in degrees, 2 degrees is the limit to avoid rate limiting)
- **Display options**: toggle visual elements

If you've made an OpenSky account (which I highly recommend), you can find your credentials under your account settings at opensky-network.org. With authentication, you get 4000 requests per day instead of 400, making the live view much more accurate. Read more about the API [here](https://opensky-network.org).

This configuration page is accessible anytime the device is connected to WiFi, so you can tweak settings whenever you want.

That's it! Once you've configured everything, you should see a live view of all flights over your location. Enjoy :)

<img width="400" alt="IMG_7935" src="https://github.com/user-attachments/assets/118b9a1c-c2c0-488d-b638-d8684a30b1d7" />

## FAQ

> the port is busy or doesn't exist


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
