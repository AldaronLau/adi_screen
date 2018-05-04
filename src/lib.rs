// lib.rs -- Aldaron's Device Interface / Screen
// Copyright (c) 2017-2018  Jeron A. Lau <jeron.lau@plopgrizzly.com>
// Licensed under the MIT LICENSE

//! Aldaron's Device Interface / Screen is a library developed by Plop Grizzly
//! for interfacing with a computer screen or phone screen to render graphics.

#![warn(missing_docs)]
#![doc(html_logo_url = "http://plopgrizzly.com/adi_screen/icon.png",
	html_favicon_url = "http://plopgrizzly.com/adi_screen/icon.ico",
	html_root_url = "http://plopgrizzly.com/adi_screen/")]

mod window;
mod sprite;
mod gui;
mod texture;
mod gpu_data;

pub use window::{ WindowBuilder, Window };
pub use sprite::{ Sprite, SpriteBuilder, SpriteList, Transform };
pub use gui::{ Text, Button as GuiButton };
pub use texture::Texture;
pub use gpu_data::{ Model, ModelBuilder, Gradient, TexCoords };

extern crate ami;
extern crate adi_gpu;
extern crate aci_png;
extern crate fonterator;

pub extern crate adi_clock;

pub use adi_gpu::{ afi, Input, Key, Click, Msg };
pub use ami::{ Mat4 };
