---
title: "Unreal Engine 5 Mist Particle System"
source: "https://www.youtube.com/watch?v=C_soSQb11fc"
author:
  - "[[CodeLikeMe]]"
published: 2022-08-10
created: 2026-10-28
description: "Project Files: https://www.patreon.com/posts/70342020 . In this tutorial, I am going to implement a mist particle using unreal engine 5 Niagara particle system. Here, we will only use what's availabl"
tags:
  - "clippings"
---
![](https://www.youtube.com/watch?v=C_soSQb11fc)

Project Files: https://www.patreon.com/posts/70342020 .  
In this tutorial, I am going to implement a mist particle using unreal engine 5 Niagara particle system. Here, we will only use what's available in the engine, so you don't need any external textures. We will use a sub uv animation inside the particle system to give a moving and changing effect to sheets of mist.  
  
► 👇 𝐒𝐮𝐩𝐩𝐨𝐫𝐭 𝐓𝐡𝐞 𝐂𝐡𝐚𝐧𝐧𝐞𝐥 // 𝐁𝐞𝐜𝐨𝐦𝐞 𝐀 𝐏𝐚𝐭𝐫𝐨𝐧!  
https://www.patreon.com/codelikeme  
►Patrons will have access to project files of all the stuff I do in the channel and other extra benefits  
  
Join this channel to get access to perks:  
https://www.youtube.com/channel/UClb6Jh9EBV7a\_Nm52Ipll\_Q/join  
  
Like my facebook page for more content : https://www.facebook.com/gamedevelopersclub/  
Follow me on twitter : https://twitter.com/CodeLikeMe2  
Follow me on reddit : https://www.reddit.com/user/codelikeme  
#CodeLikeMe #unrealengine #ue4 #indiegamedev

## Transcript

**0:00** · hello everyone so today we are going to implement this mist effect in unreal engine 5 using niagara effect system so yeah actually when i was working on the snow particle effect i saw some people have commented that they would like to see a missed effect so here i thought i've implemented it because i

**0:26** · haven't really tried to implement an effect like this before so anyway this is not a not a very complicated particle system so yeah let's see how to implement this today

**0:42** · okay so just like always let's create a new niagara system in the effects folder here i'll choose again this hanging particles and s mist okay so this is what we get let me just put it here

**1:16** · right so then uh first in order to have missed we need to have a have big particles so first let me

**1:32** · \[Music\] change this spawning area i'm not going to use box i'm going to use this shape sphere let's try 10 and 10 10 by 10 for the num in uniform x and y square so they will be spawned in a wider area around this origin and \[Music\]

**2:05** · then i'll make this the size as for the lifetime this should be fine so size let's use something like 20 30.

**2:27** · so now you get much bigger particles but still not big enough so how should we maybe this should be more bigger than this so

**2:52** · about 200 to 300 okay that seems fine so now the next step is they don't really look like mist they do but we can still see these sphere shapes which is not what i need so if you enable

**3:19** · show engine content oh i disabled it i have already enabled it if enabled you can see the contents assets from the engine so here if you type smoke you have this m smoke sub uv blackbody material

**3:42** · so which is located here but this is not actually the material i'm going to use but let's just try it out and you can see so you can see this uh something like an array of smokes smoke sprites so here we have eight and eight by eight array

**4:10** · sub uv so smog let's try right so if i change this sub image size to 8 by 8 you will get only one of them now see you get this smoke like

**4:26** · appearance but it's black so let's create a new folder here mist and move mist into this and also let's create a copy of this material

**4:53** · inside the mist folder copy here make sure to copy don't move because this needed to be stay without any changes in the engine content otherwise it can affect other projects so um smoke

**5:12** · sub pov mist that's a suitable name so here now we have this uh dynamic parameter and blackbody \[Music\] let's just

**5:33** · delete this part \[Music\] and multiply with the particle color so that we should get \[Music\] white smoke like appearance oh wait

**5:50** · i need to assign this material here \[Music\] okay now we are starting to get the effect we need but you can see uh it's not actually it doesn't look like mist so maybe we should increase this phone rate let's try 500

**6:27** · probability one okay now you see some flickering effect that's because this is in translucent let's try additive

**6:46** · no that's that suddenly becomes much nicer so yeah now this is working so i have limited the exporting area to this

**7:02** · but if you want you can stick it to the player character so when the player moves around the missed particle will stay with the player and one more thing even though we can see some moment

**7:21** · in the mist it's it doesn't really we don't really see any change in the texture but if we enable uh sub uv animations we should be able to see that some change in effect how uh here sub u v animation so starting from 0 and frame is 63

**7:55** · so now you can see it is changing see so it's now looks more like real see here you can see the mysteries change um if you

**8:16** · want to slow down this effect you can multiply float and use the normalized edge if you use a multiplier like 0.2 you will see you can slow down the change if you use a higher number you will see them is changing fast see but i guess one is good

**8:44** · so all right so here is the final effect and one issue that i noticed with this method is we can see these sharp edges where this mist sprite cut off other objects so that is a problem that i have with the system so at the

**9:06** · moment i don't have a solution for that but i will look into any solution so methods to fix it and once i know i will share it with you guys also if you want to move up the spawning area you can either move the mist up or you can add offset here

**9:36** · like 50 whatever you wish whatever you like so yeah actually when you lift them you can minimize this cut-off region with the ground so it looks much better

**9:55** · so yeah i guess that's it for this video so thanks for watching as always project files will be available here for the download in the patreon page link could be in the description below and if you like to support my work you can get the membership of the patreon club see you in another episode goodbye \[Music\]