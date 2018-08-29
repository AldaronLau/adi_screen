// Copyright Jeron A. Lau 2017 - 2018.
// Dual-licensed under either the MIT License or the Boost Software License,
// Version 1.0.  (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

//! Render graphics to a computer or phone screen, and get input.  Great for
//! both video games and apps!

#![warn(missing_docs)]
#![doc(html_logo_url = "http://plopgrizzly.com/adi_screen/icon.png",
	html_favicon_url = "http://plopgrizzly.com/adi_screen/icon.ico",
	html_root_url = "http://plopgrizzly.com/adi_screen/")]

/*mod window;
mod sprite;
mod gui;
mod texture;
#[doc(hidden)]
pub mod prelude;*/

/*pub use prelude::{ Transform, Vec3 };
pub use window::{ Window, Widget };
pub use sprite::{ Sprite };
pub use texture::Texture;
pub use gpu_data::{ Model, ModelBuilder };*/

mod gpu_data;

pub mod prelude;

extern crate awi;
extern crate aci_png;
extern crate fonterator;

pub use awi::screen::{Screen, ScreenError, Shape, Gradient, Model, Texture, TexCoords};
pub use awi::render::{Event, Mat4, Transform};
pub use awi::render::afi::*;
