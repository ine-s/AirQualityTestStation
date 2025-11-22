#  Air Quality Monitoring Station  
Station complète de mesure de la qualité de l’air basée sur **PMS7003**, **BME280** et un **anneau NeoPixel**.  
Le système lit les paramètres environnementaux, corrige la mesure du PM2.5 en fonction des conditions ambiantes, puis affiche la qualité de l’air et la température de manière visuelle.

---

##  Matériel utilisé
- **Arduino Nano Every**  
- **PMS7003** — particules fines PM1.0 / PM2.5 / PM10  
- **BME280** — température, humidité, pression  
- **NeoPixel Ring (12 LEDs RGB)**  

---

##  Principe de fonctionnement
1. Lecture des valeurs du **BME280** (température, humidité, pression).  
2. Lecture et décodage des trames du **PMS7003** via UART.  
3. **Correction du PM2.5** selon la température et l'humidité (basée sur les recommandations Plantower).  
4. Classification de la qualité de l’air selon les seuils EPA.  
5. Affichage sur **NeoPixel** :
   - **Couleur** → niveau de pollution  
   - **Nombre de LEDs** → température  

---

##  Visualisation LED
- **Vert** : air sain  
- **Jaune** : légère pollution  
- **Orange** : pollution modérée  
- **Rouge** : air pollué  
- **Violet** : pollution très élevée  

Nombre de LEDs allumées = indicateur de température (2 → froid, 12 → chaud).

---
