# CSSense-CLI
Lovesense Gamestate Integration for CSGO

## What does it do?
If it hasn't been clear already let me explain it to you.
[Lovesense](https://lovense.com/) is a famous brand for remote controlled sex toys.
With this tool you can play CSGO and have some additional fun (╭ರᴥ•́).

### Settings & Modes
- **Base Vibration** is the vibration strength that's present all the times when ingame. It's an integer ranging from 0 to 20.
- **Increase on Kill** makes kills increase (or decrease) the vibration strength by a certain value.
  - **Increase Amount** is the value used in the aforementioned mode. It's a double ranging from -20 to 20.
  - **Vibrate on Shoot** is mode in which the increased value is not used all the time but only when a weapon is currently being shot. If one doesn't shoot for ~100ms then the vibration will return to the base strength.
  - **Stop on Knife** temporarily sets the vibration strength to it's base as long as the player's knife is out.
- **Burst on MVP** will set the vibration strength to it's maximum when the player got awarded as MVP.


In the following video you can see what it would do when connected to a device (using default settings):

https://user-images.githubusercontent.com/17300231/123869009-bd266600-d930-11eb-9af3-c42f64df2c8b.mp4

The may be some delay in the console which is not present when running normally.

### Notes
The CSGO gamestate integration is limited and thus some things currently can't be detected easily.
If a player dies only base vibration gets applied. Using a bot will not change this.
It will falsely detect a weapon as being shot if you pick up the same weapon but with 1 bullet less in the magazin from the ground.

It can cause quite the traffic on your PC.
To lower the amount of traffic you can finetune throttle and buffer, but this will lead to delays or even break things.

It currently only connects to newer Lovesense devices but this can be changed by updating the regex to include the old services or by matching them using their names. As I don't have any of these old devices I was not able to test this and thus did not include it.

## Setup
Either compile it yourself or download the pre-built binaries from the releases page.
Run `CSSense -i` to configure it and then start CSGO.
After having it configured you can just run it using `CSSense`.
You can run it after CSGO is running, as long as it has been configured before.

https://user-images.githubusercontent.com/17300231/123868556-28bc0380-d930-11eb-89ae-db828f30c01c.mp4

### Important
Don't double click the ".exe". Instead `cd` to the directory holding it in your terminal and enter the commands there.
It's also important to not remove the ".dll" shipped with it.

## Why did you make this?
Fun.

## Why didn't you use their integrations?
Lovesense's toys already have many integrations and are really customizable, but I thought it would be fun connecting it to CSGO.
I do not like having to rely on Lovesense Remote nor do I like connecting to the device over the internet, so I directly connected to the device using bluetooth low energy.

## Future ideas
- A new mode called "Dynamic Pattern" which changes the strength based on the currently active weapon when "Increase on Kill" is turned off.
- Integrate more things such as hostage being taken, bomb ticking and exploding or player health.
- Use the programmable pattern feature Lovesense toys have.

## Disclaimer
The code is really messy as this is what I consider a "meme" project, so don't use this as a reference for something.
