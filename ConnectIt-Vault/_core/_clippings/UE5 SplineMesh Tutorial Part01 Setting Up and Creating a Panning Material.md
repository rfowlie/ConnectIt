---
title: "UE5 SplineMesh Tutorial Part01: Setting Up and Creating a Panning Material"
source: "https://www.youtube.com/watch?v=ilmIJxRqqEo"
author:
  - "[[UECore]]"
published: 2022-08-10
created: 2026-10-28
description: "Hi everyone! Welcome to another Unreal tutorial. This time, I will be going through how to create a spline mesh blueprint system. The tutorial will be split into 2 parts, with this first part being se"
tags:
  - "clippings"
---
![](https://www.youtube.com/watch?v=ilmIJxRqqEo)

Hi everyone! Welcome to another Unreal tutorial. This time, I will be going through how to create a spline mesh blueprint system. The tutorial will be split into 2 parts, with this first part being setting up and creating a material for the spline mesh. Hope everyone can learn something new from this tutorial and thank you for watching!  
  
If you would like to get a copy of the fbx and texture file for the spline mesh, you can get it from this link below:  
https://drive.google.com/file/d/1Ku6MXipeE0YqIYtt\_p7a0UskIXINAKyF/view?usp=sharing  
  
#unrealengine5 #unrealtutorial  
  
\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_  
If you like what I am doing and would like to support me, you can do so from my Ko-Fi page. Also thanks for the support :)  
https://ko-fi.com/unrealwithkailin

## Transcript

### Intro

**0:00** · Hi everyone! Welcome to my tutorial on creating a simple spline mesh system And once again you can follow along with this tutorial with either the Unreal Engine 5 or Unreal Engine 4.

**0:11** · So for my example, I have this spline mesh And it's currently an arrow that's a panning texture arrow And what you can do is you can extend the spline mesh by pressing Alt and holding down Alt and on the last point you can drag it out to make another point and you can pretty much just like...

**0:36** · tweak it into ways that you want it to be For example, like a different road systems or like a waypoint system for players to know where to go and this can be pretty handy for like maybe tower defense games or racing games where you have walls of arrows to point where the cars are supposed to go

**1:02** · or to make like simple roads and stuff just exchange this texture into a road texture and that'll be fine so with this system you can also go into the details and there are other different parameters you can change Here I have a parameter for color and for glow and for speed so for color, you can simply just like change the color of the arrows on that system

**1:29** · and for glow, I can increase the glow or decrease it depending on what I want and for speed, it's just pretty much how fast the panning for the texture is going to be like yeah so I can do it really fast or I can do it like slow

**1:47** · and make it back go backwards if I want to So that's generally what I'm going to teach in this tutorial I'm going to split it into two parts, one is the making the material and the other one is making the blueprint for the spline mesh and like before, I will just like delete whatever I have here and restart from scratch so I can teach you guys step by step process of how I created this spline mesh system All right!

### Setup

**2:15** · So starting from scratch, I'm going to create a new folder and I'm going to call it BP\_SplineMesh and within this folder we will need to import the mesh itself and the texture that we're gonna use so I'm going to go ahead and right click and go to import and I'm going to choose the 3d mesh that I'm going to use and import it in

**2:41** · I can leave everything on default and then just import it in and then, I'm going to go ahead and import the texture itself as well the mesh I'm using is pretty simple it's just pretty much a plane

**3:01** · If I turn on the wireframe of the plane so you can see that for like a simple plane it's a There's quite a few polycount in this plane and the reason is that we need enough segments in this plane in order for the spline mesh to deform and bend smoothly and also keep in mind that the higher poly count you have the more it might take away from your computer's processing power so you want to find a balance to that and not go too crazy on the polycount

**3:31** · As for the UV for the plane, it's a very simple UV I pretty much just quickly edit it and put it on the bottom of the UV and so my texture would be like this where the arrows will be on the bottom half of the texture and yours will probably be different from mine depending on what kind of model you use and what kind of UV you've done on the model Next up I'm going to create the material for the panning arrow So I'm going to right click and create a new material I'm gonna name this material as m\_PanningArrow

**4:06** · Let's hop into it and in the material graph Click on the physical material and we're gonna change the blend mode into translucent and check the two-sided Within this material, we are basically gonna use the base color, the emissive color and the opacity and that's pretty much the only three that we're going to touch in this tutorial For the base color, I'm going to right click and search for a vector parameter

### Panning Material

**4:35** · and I'll name this vector parameter as Color There's also a shortcut for the vector parameter which you can hold down v on the keyboard and press the left mouse button (V + Left Mouse) Comes in pretty handy and if you didn't know now you know Next I'm going to search for texture sample node So that I can use the arrow texture that I have and the shortcut for this is a T + Left Mouse button

**5:02** · Okay, so I'm going to use my arrow texture that I have onto the texture sample and I'm gonna do a multiply (Shortcut is M + Left Mouse) and I will take the alpha of the texture sample into b Then let's link the result into the base color Apply and save

**5:29** · And right now in the preview it's all black because we don't have the opacity in yet so let's drag the alpha of the texture into the opacity and you should see the arrows...

**5:40** · in the texture preview and I'm going to go ahead and change the color into yellow and you can change to whatever color you want So now we see the arrows and the material and we can change the color but it's not moving so how do we make it so that the texture can move It's actually pretty simple All we need is a panner node Let's go ahead and right click and search for panner

**6:08** · add that in, and if you hover over the node and press and hold Alt + Ctrl at the same time, Unreal will let you know what this node does and what the panner node does pretty much is it will take the U coordinate and the V coordinate of the UV and then just move it around to make a panning moving kind of effect so within this node we only need the coordinate and the speed so let's right click and search for texture coordinate and then hook that up to the coordinate pin we can leave this as default if you want to change the UV tiling you could do so

**6:42** · to get another like different kind of textured effect texture sizing if you will and for the speed we need a vector 2 constant so I'm going to search for vector vector 2 oh my bad it's constant 2 vector

**7:02** · The shortcut for this is if you press on 2 + left click you can call the constant two vector so let's hook that up to the speed Now if you look at the details there's R and G R pretty much controls the U coordinate of the UV tiling So if I do like a negative 2 it will go left to right as you can see on the preview

**7:28** · and if I change the value of G to like 1 it will go from down to up and this is how we generally want to use to control the panning of our material For my R, I'm going to use a negative number because if I use a positive number like 0.55

**7:45** · you can see that it's panning in the wrong direction, so keep in mind when you do this just make sure like the direction that you're using is the correct way Here I'm going to use a -0.2 and you can see that the panning direction is the way that I want I'm going to go apply and save first and I'll be right back Okay, the last thing we want to do with the material is to add a glow Hold down S + left click to add in a scalar parameter and let's call this Glow

**8:17** · and you can also right click and search for scalar parameter as well so that works too Right here to have the material glowing properly, we need to multiply the glow with the output of the base color and the texture sample so let's search for multiply and connect the glow into the B and then connect the output into the emissive color

**8:45** · and with the higher value, the more the glow so if we bump up the value, it'll be more intense and if we push it put it more down then there will be less glow like so All right so, actually let me show you why we want to use the multiplier instead of just simply attaching it so with this attached, if I just simply change the default value to like maybe like a 2

**9:10** · you can see that it's completely white already so that's not what we wanted and it seems like the emissive is just overtaking the color of the material and making everything white and we need to make the value very small in order to see our color that's why we need this multiply to have everything working properly as we intended to do All right!

**9:33** · So I'm going to bump up the value to around like 2.6 and then go ahead and apply and save All right, let's close this down and in the main scene, let's drag out the plane into the scene and let's test the material onto the plane so let's change the material into the panning arrow and Boom! We have the panning material onto the plane and right now the glow is kind of small so let's...

**10:05** · I'm going to go back into the material and just change... bump it up into like 20 Yeah And there! There's the glow that we want Actually I'm going to go back to the material and show you guys why we need to multiply so if I do it like this, you can see that the color is gone and it's all white so that's why

**10:30** · we want to have the multiplier Sorry, it's probably better to show here instead, so there's that And if you're new to Unreal, if you don't see any glow That's probably because you don't have a post processing volume in the scene To add that simply go to the place actors and search for a post processing volume

**10:51** · and just drag it into the scene In the details of the post processing volume, there's a few settings that we want to set up so first we want to have the infinite extent checked Hold on let me see where it is

**11:08** · Here it is so have this checked, if it's unchecked then everything will be bound inside the box so you want to have this checked As you can see Another setting that we want, if we scroll up Head to bloom At first it will be unchecked, So you can check those up and then change the intensity the higher the intensity the more bloom you can see in the screen

**11:36** · and I'm just gonna keep it at 0.7 for now and the next thing is lens flare By default, intensity will be at 1 and it'll look something like this which is quite a lot of lens flare in my opinion so i'm gonna just keep it at 0 Okay! So that's pretty much how you make the material for the spline mesh that we are doing And in the next part of this tutorial I will show you guys how to make the blueprint for spline mesh Thank you everyone for watching!

**12:06** · Hopefully you learnt something new from this part of the tutorial and also never stop learning