---
title: "Unreal Engine 5 Mana Drain UI - Create Professional VFX Materials Like a Pro"
source: "https://www.youtube.com/watch?v=ANn-AnvxQ8E"
author:
  - "[[CGHOW]]"
published: 2025-06-20
created: 2026-10-28
description: "Learn to create a fully customizable Mana Drain UI widget in Unreal Engine 5 using the powerful Material Editor! In this comprehensive tutorial, your guide, Ashif Ali, a RealtimeVFX artist and YouTube"
tags:
  - "clippings"
---
![](https://www.youtube.com/watch?v=ANn-AnvxQ8E)

Learn to create a fully customizable Mana Drain UI widget in Unreal Engine 5 using the powerful Material Editor! In this comprehensive tutorial, your guide, Ashif Ali, a RealtimeVFX artist and YouTuber from India, will teach you how to build a professional-quality UI element with dynamic parameters for drain effects, full color control, and advanced material setup. This video is perfect for game developers and VFX artists looking to master real-time UI materials and create professional, polished game interfaces.  
  
We will dive deep into the Unreal Engine Material Editor to build this versatile asset from scratch. You'll gain a deep understanding of advanced material nodes and how to use them to create a convincing liquid drain effect, with full control over every aspect. This powerful workflow will allow you to link your UI directly to your gameplay logic, creating a truly immersive experience for your players.  
  
What You'll Learn in this Tutorial:  
🎯 Material-Based UI Widget Creation: Master the essential nodes and techniques needed to create a dynamic UI widget entirely within the Material Editor, allowing for more creative and efficient workflows.  
  
🎯 Dynamic Drain Effect Parameters: Discover how to set up parameters that give you complete control over the speed and style of your mana drain effect. You'll learn how to make the liquid recede smoothly or with a powerful, fast-draining animation.  
  
🎯 Color and Tiling Customization: Learn to create a dynamic color system that allows your mana bar to change color in real-time based on its value, from a vibrant blue for full mana to an ominous purple for low mana. 💜💙  
  
🎯 Advanced UE5 Material Editor Techniques: I'll show you how to use advanced material nodes and functions to create the illusion of fluid dynamics, with subtle ripples and a convincing liquid surface.  
  
Keywords & Information:  
Tutorial Level: Beginner  
  
Software: Unreal Engine 5 (UE5)  
  
Keywords: Unreal Engine 5, UE5, UE5 Niagara, Niagara tutorial, Healing VFX, Healing Beam, Magical VFX, RealtimeVFX, Game VFX, Unreal Engine tutorial, VFX tutorial, Game art, UE5 particle system, Niagara particle effect, visual effects, CGHOW, Ashif Ali.  
  
Socials & Call-to-Action:  
I hope you found this tutorial helpful for your projects! If you enjoyed learning how to create this Mana Drain UI, please hit the like button 👍 and subscribe to my channel, CGHOW, for more Unreal Engine tutorials. 🔔  
  
Course: https://youtu.be/DZnuHK1Wckc  
  
Linktree: https://linktr.ee/cghow  
  
Website: https://cghow.com/  
  
ArtStation: https://www.artstation.com/ashif  
  
Twitter: https://x.com/cghow\_  
  
Instagram: https://www.instagram.com/cghow.com\_/  
  
#UnrealEngine #RealtimeVFX #uieffects  
Unreal Engine 5 Mana Drain UI tutorial  
dynamic progress bar material UE5  
UE5 stylized resource bar VFX  
creating dissolve effects in Unreal Engine materials  
advanced UI shader tutorial Unreal Engine 5  
Unreal Engine 5 fluid UI material effects  
how to make a dissolving mana bar in UE5  
animating UI textures with panners in Unreal Engine  
adding edge shading and depth to UI materials  
how to fix sharp edges on material dissolves UE5  
creating multi-channel UI textures for game dev  
dynamic color shifting for resource bars in UE5  
UE5 Material Editor SmoothStep node logic  
procedural noise textures for game UI  
UV distortion for fluid motion in UE5  
Unreal Engine 5 UI mask texture tutorial  
customizing progress bar materials in Unreal  
UE5 material parameter collection for UI  
  
  
0:00 Intro & Final Effect Preview: Complex Fluid UI Bar Material ✨  
0:10 Importing & Reviewing Source Textures (Frame & Channel Mask)  
0:37 Setting up Base Material (Unlit, Translucent) and Frame Texture  
1:20 Implementing Scale UV & Tiling Parameters (Tile\_XY)  
2:18 Matching the Aspect Ratio & Using Clamp to Cut the Shape  
3:12 Applying the Inner Noise Texture and Color  
4:35 Adding Tile Inside Parameter for Inner Noise Texture  
5:09 Multiplying with Inside Color Parameter (Blue Default)  
5:30 Creating the Wipe/Dissolve Mask using Gradient & Smooth Step  
6:20 Setting up Min and Max Parameters for Edge Sharpness Control  
6:47 Inverting Gradient & Adjusting Masking Logic (Add/Multiply)  
8:10 Refining Dissolve: Sharper Edge Control and Noise Tiling Adjustment  
8:50 Adding Depth/Shading: Using Green Channel Mask & Lerp  
9:37 Introducing Edge Shade and Inside Shade Parameters for Depth  
10:21 Adding Power Control for Edge Contrast/Roundness  
11:07 Animating the Inner Noise Texture (Panner & Noise Pan Parameter) 💨  
12:22 Adding Fluid Distortion: Blurred Noise & Distort Parameter  
14:19 Adding Animated Stars/Dots Texture for Extra Sparkle ✨  
15:09 Implementing the Chaos Node/Motion Function for Random Particle Movement  
16:01 Adding Star Color and Connecting Star Layer to the Final Material  
17:10 Finalizing Star Blending (Fixing Masking and Saturation Issues)  
20:00 Final Demo and Material Review

## Transcript

### Intro & Final Effect Preview: Complex Fluid UI Bar Material

**0:00** · Hi guys, I'm Asi and watching s job video. Today we will make this cool material inside Unreal Engine 5. So let's dive right in and start this. So let's start this. So I have this empty folder and here I will import two texture. Let me show you which one. This and this. And let me show you what is.

### Importing & Reviewing Source Textures (Frame & Channel Mask)

**0:19** · So this frame is just this frame which has transparent PG. And then we have this inside and it has channels. So you can see it has this and this channel. Okay. So this flat and this green for the edge. So let's make a material for it. So I'll call this M.

### Setting up Base Material (Unlit, Translucent) and Frame Texture

**0:45** · Okay. So I'll just drag both from here to here. And let's make this flat planer. And I'll use uh unlit And also I'll use uh let it be unlit for now. So we have now this frame which we can connect here. Okay. So now you can see how it looks. So we can multiply this frame with the alpha of it. So it will be like this now.

**1:17** · And because of the uh planer it's just uh projecting on full planer. So we need to scale it down uh how much we need. So I'll just take the scale UV and just take the texture coordinate here and we need two vector here. So I'll just take this and make this actually parameter.

### Implementing Scale UV & Tiling Parameters (Tile\_XY)

**1:44** · I'll call it tile. And here if I go down you can call X and Y.

**1:50** · Okay.

**1:52** · So you have X and Y. Then I'll use this and put it here. By default, it should be like one and so first thing I'll just save it and make an stance of this and then I'll set the tiling uh the size actually from this. So it is like this and then it should be like this.

### Matching the Aspect Ratio & Using Clamp to Cut the Shape

**2:18** · So basically if I uh if I show you the texture size which is this. So you can use the same ratio if you want. So this is 15 uh and this is four. So it is almost three times width. So now you can see it's almost three time width. Okay. And then it should cut these both. So you can use clamp if you want and you can do also one thing.

**2:50** · So if you use this clamp it will just cut it out. So I'll just save.

**2:58** · Now it's cutting now. And I'll put the same value. Let's make it one and 35 I guess. Yeah. So I'll use the same values here.

**3:07** · 1 and 35.

**3:10** · Okay. So we have this. Now same tiling will be for this because this is what we will use. So basically if you now want this to be a white or any color what you can do let's say you want white so you can just add this red channel on it. So it will be right uh white because it has that thing and this need to be also clever.

### Applying the Inner Noise Texture and Color

**3:38** · Okay. Now you can use any color or you can any use any texture if you want.

**3:45** · So let's say you have let me save all these.

**3:49** · I have lots of lots of noise texture. If I just type here noise and search texture I have lots of textes. And if you want these I have already this package of these noise text on my Patreon and I have lots of tutorials also uh how to make these kind of thing.

**4:08** · So, let's say just for fun, I'll use maybe this one.

**4:16** · And I'll just use this here.

**4:20** · Okay. And just multiply this. Uh let's move it here. And multiply it with this.

**4:28** · And then here. So, it depends whatever you will multiply here, it will be here.

**4:34** · Okay. And then for the tiling of this, so I'll just call it x coordinate and it has uh we can just duplicate this and we can call it tile inside.

### Adding Tile Inside Parameter for Inner Noise Texture

**4:50** · Okay, I'll put one and one both here.

**4:53** · Sift C and then multiply it with this.

**5:03** · Okay. So, we can adjust that outside.

**5:07** · Now, if you want uh let's say you want a color. So, I'll just multiply with the color three vector. And also, let's make this parameter. So, this will be inside color.

### Multiplying with Inside Color Parameter (Blue Default)

**5:21** · And let's say by default it is blue.

**5:26** · It will be here. So, you can change it later if you want. Then we want to dissolve it. And to dissolve it, there are lots of ways to do it. You can use the same texture to dissolve. You can use a separate texture to dissolve this texture. So let's say you have this texture coordinate and you have a mask red which is a gradient like this. And then I'll you I'll just multiply it with this same texture.

### Creating the Wipe/Dissolve Mask using Gradient & Smooth Step

**5:55** · And then uh instead of connecting it here, I will connect it here. But before that, I'll take the smooth step which is the best thing to best node to dissolve anything because it will give you the control how much smoothness or sharpness you want in your dissolve. So I'll connect it here and then this thing will go here. Now I'll use parameters.

### Setting up Min and Max Parameters for Edge Sharpness Control

**6:24** · So this is our minimum and this is our maximum. Maximum should always be bigger than minimum. So I will just add the maximum on the minimum.

**6:36** · Okay. Now you have this setup. Let's uh not preview this and let's just make this zero and one and let's save it. We need to invert the gradient because it is dissolving from here. Let me show you.

### Inverting Gradient & Adjusting Masking Logic (Add/Multiply)

**6:52** · So basically if I increase or decrease this value you can see and if I decrease this value it will make it sharper. So you can see it's now sharper and this dissolving whole thing together. So for that first we need gradient from here to here. So for that I will invert this gradient and it's dissolving this whole thing together. It should be like a wipe. So for that instead of multiplying it delete it I will add this on it and that's it.

**7:25** · Now let's save it and here this is the fadness.

**7:42** · If you make it smaller value it will look sharper. You make a bigger value it will look softer.

**7:51** · Still it has issue. See it's dissolving but this part is remain here. So you can do one more thing which is after adding it you can now multiply it again and that should resolve the issue.

**8:09** · Yeah. Now it resolved the issue and you can make it sharper. And now we have this inside tile. So if I make it five and five now you will have better idea how it's going. So with this you can make it softer or harder. And we can do tiling. Maybe we want like this.

### Refining Dissolve: Sharper Edge Control and Noise Tiling Adjustment

**8:36** · And this should be like this.

**8:39** · So it depends how you want it.

**8:43** · And it's already looking cool.

**8:47** · Now like this thing looks flat. So to make it uh little more uh add a little more depth in it. We have another uh channel here green channel. So to make it overall darker or lighter what I will do I will do. I'll make a lurp in this green channel. I will use and then I will multiply this whole thing with this and this L will give me the control uh on this. So let me show you.

### Adding Depth/Shading: Using Green Channel Mask & Lerp

**9:21** · So because this is white and black. So L will give me control if I want to change the this part or this part. Okay. So white means this one part and black uh black means this a. So let's make these all also parameter edge shade.

### Introducing Edge Shade and Inside Shade Parameters for Depth

**9:45** · By default I'll put the one inside shade. You can call whatever you want. I'll just put the one one for both. Save.

**10:00** · And now with these two if I decrease the value you can see the inside part is getting darker. And if I decrease this value you can see the outside part and the edge part is getting darker.

**10:13** · Okay.

**10:15** · So it gives little bit roundness.

### Adding Power Control for Edge Contrast/Roundness

**10:21** · Although you can also use a power to control it little more.

**10:28** · I'll just call it power. Put the default value one.

**10:33** · So we can just tweak that little bit. So now you can see it's making it let's say it's.5 and it's brighter. Let's say so now this power will give me the control to make this like this or this.

**10:52** · And now it looks much better and it's dissolving.

### Animating the Inner Noise Texture (Panner & Noise Pan Parameter)

**11:07** · Now you can use little uh you can add some uh motion here and you can also add motion in this one. So for that in this one you just need to pan it and I'll just copy this and connect here. This will be noise pan and this will be X and Y. So let's save it.

**11:39** · So you can now decide on which axis you want to pan it. So you need to make it real time and let me check from where we can make it real time because things has been changed in Android 5.6.

**11:59** · Yeah, this is real time. Now you can see it's animating and it's diagonal because X and Y. Now it's going up. I don't want that to go up. It looks better. 8.2 maybe. Yeah, this one is.3.

**12:19** · This one is better. Cool. Now we want to add little motion and we can also dissolve it from here. So it will be more fluidy. So to do that I'll just make some space here. This is a noise texture we have.

### Adding Fluid Distortion: Blurred Noise & Distort Parameter

**12:36** · So all I do is I'll take one more texture and this time I'll take the blur noise this one. And what I'll do I'll use all things from here. I I'll just use a different panel.

**12:57** · I'll use it here and speed. use the same UVs. It just it should be little bit more faster. So I will add 2 here. So whatever panel speed I have, it will be faster. Then connect here.

**13:19** · Then I will add this noise texture to the UVs of this texture. And before that I will multiply to control how much I want. So let's call it distortion or distort whatever by default zero. So you will see the difference once I'll do it. Now you can see how it going. And now if I change the value now it's fluy kind of thing.

**13:45** · But you have to make it uh how much you want to not that not too much just slightly like this much or maybe just 0.1 little bit.

**14:04** · Okay. So we can make it tile maybe just three and three.

**14:14** · Yeah, much better. So now we want to add something here also. So let me save again.

### Adding Animated Stars/Dots Texture for Extra Sparkle

**14:21** · Let me find a texture which is called star.

**14:27** · I have a star texture.

**14:31** · Let me find here.

**14:36** · This one. So it has these dots and it has three channels. So which gives more variation. So, I'll just use this texture here.

**14:47** · And if I preview and we can pan it on one direction and I will pan it uh on this direction. But I want more motion in this.

### Implementing the Chaos Node/Motion Function for Random Particle Movement

**15:09** · So what I can use type here chaos you don't need this node but go inside to this function copy this node motion for copy close this and just paste it here.

**15:26** · Now make your texture an object and connect your this here. If I preview now you see the motion and now you can add your pen also. So it will be a directional but it is also have uh all all direction motion.

**15:44** · Okay. Now we need to add all these channels. So we can do is uh break three and then add these together.

**15:58** · So now we have this white.

### Adding Star Color and Connecting Star Layer to the Final Material

**16:01** · Now we can multiply it with a color.

**16:08** · Let's say stars and then we can add it on here.

**16:19** · So you can add it. Uh let me make some space actually.

**16:24** · So this whole thing is noise. Then we have this for the edge.

**16:32** · Then we have this.

**16:34** · Make some space here and here also. Pl this. So basically this is the texture which is masking it all. So we need to add it here.

**16:55** · Let's save it.

**16:59** · And now if I increase the color, let's say if this is red not zero.

**17:08** · Let me see why.

### Finalizing Star Blending (Fixing Masking and Saturation Issues)

**17:12** · Let's make this off because it is on the other side. So what I'll do because we are also dissolving all things all. So, I'll just connect this here. And what I'll do actually uh actually what I'll do, I'll just delete this.

**17:41** · I'll just add this here before it because smooth this tab is dissolving it. So, we need to add it here.

**17:54** · And now this will be our star shade.

**18:01** · How much brightness we need in the star?

**18:03** · It should work.

**18:11** · Yeah, but it's working on different side. Although I I'm liking it.

**18:16** · I'm liking it actually.

**18:22** · I'm actually liking it. But price on the other side, it should be.

**18:45** · Let me just saturate it first. Some issue.

**19:05** · It's behaving differently.

**19:20** · Yeah. Now it's on both side.

**19:29** · What we can do now? We can make this uh lesser here. So what I'll do I'll actually we need to saturate this value. It will fix this because maybe some values are uh in negative.

**19:50** · And now I'll use this and We need to multiply it then we need to add it here.

### Final Demo and Material Review

**20:09** · So it will be mask through it then add like this. So now we can increase it even more if you want. And this is how it works.

**20:24** · Looks cool. And I'll make it little fade.

**20:34** · Looks nice. You can do a lot of thing in this one. But I hope this is this is it.

**20:41** · So, thanks for watching. Keep learning and bye-bye.