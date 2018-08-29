// Copyright Jeron A. Lau 2017 - 2018.
// Dual-licensed under either the MIT License or the Boost Software License,
// Version 1.0.  (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

pub use awi::screen::PathOp3D;
pub use prelude::PathOp3D::{Move, Line, Quad};
pub use awi::render::*;
pub use gpu_data::*;

/// Shape attachments.
pub enum Atch<'a> {
	/// color, blending
	Solid([f32; 4], bool),
	/// Model, gradient, blending
	Gradient(Gradient, bool),
	/// Model, texture, texture coordinates, blending
	Texture(&'a Texture, TexCoords, bool),
	/// Model, texture, texture coordinates, fade
	Faded(&'a Texture, TexCoords, f32),
	/// Model, texture, texture coordinates, tint color, blending
	Tinted(&'a Texture, TexCoords, [f32; 4], bool),
	/// Model, texture, texture coordinates, gradient, blending
	Complex(&'a Texture, TexCoords, Gradient, bool),
	/// Model, color, blending
	FoggySolid([f32; 4], bool),
	/// Model, gradient, blending
	FoggyGradient(Gradient, bool),
	/// Model, texture, texture coordinates, blending
	FoggyTexture(&'a Texture, TexCoords, bool),
	/// Model, texture, texture coordinates, fade
	FoggyFaded(&'a Texture, TexCoords, f32),
	/// Model, texture, texture coordinates, tint color, blending
	FoggyTinted(&'a Texture, TexCoords, [f32; 4], bool),
	/// Model, texture, texture coordinates, gradient, blending
	FoggyComplex(&'a Texture, TexCoords, Gradient, bool),
}

/// Macro to create multiple sprites in an array.
///
/// # Example
/// ```
/// let mut window = WindowBuilder::new("Window Name", None).finish();
/// 
/// textures!(window, aci_png::decode,
/// 	"res/texture0.png", // 0
/// 	"res/texture1.png", // 1
/// );
/// 
/// models!(window, "res/model.data");
/// 
/// let sprites = sprites!(window,
/// 		(0/*model 0*/, Some(0/*texture 0*/),
/// 	Transform::new().translate(0.0, -0.5, 2.0), false),
/// 		(0/*model 0*/, Some(0/*texture 0*/),
/// 	Transform::new().translate(0.0, -4.5, 2.0), false));
/// ```
#[macro_export] macro_rules! shapes {
	($window:expr, $( $x:expr ),*) => { {
		let window = $window;

		[ $( match $x.2 {
			$crate::prelude::Atch::Solid(color, blend) => {
				window.shape_solid($x.0, $x.1, color, blend, false, true)
			}
			$crate::prelude::Atch::Gradient(gradient, blend) => {
				window.shape_gradient($x.0, $x.1, gradient, blend, false, true)
			}
			$crate::prelude::Atch::Texture(texture, tc, blend) => {
				window.shape_texture($x.0, $x.1, texture, tc, blend, false, true)
			}
			$crate::prelude::Atch::Faded(texture, tc, fade) => {
				window.shape_faded($x.0, $x.1, texture, tc, fade, false, true)
			}
			$crate::prelude::Atch::Tinted(texture, tc, color, blend) => {
				window.shape_tinted($x.0, $x.1, texture, tc, color, blend, false, true)
			}
			$crate::prelude::Atch::Complex(texture, tc, gradient, blend) => {
				window.shape_complex($x.0, $x.1, texture, tc, gradient, blend, false, true)
			}
			$crate::prelude::Atch::FoggySolid(color, blend) => {
				window.shape_solid($x.0, $x.1, color, blend, true, true)
			}
			$crate::prelude::Atch::FoggyGradient(gradient, blend) => {
				window.shape_gradient($x.0, $x.1, gradient, blend, true, true)
			}
			$crate::prelude::Atch::FoggyTexture(texture, tc, blend) => {
				window.shape_texture($x.0, $x.1, texture, tc, blend, true, true)
			}
			$crate::prelude::Atch::FoggyFaded(texture, tc, fade) => {
				window.shape_faded($x.0, $x.1, texture, tc, fade, true, true)
			}
			$crate::prelude::Atch::FoggyTinted(texture, tc, color, blend) => {
				window.shape_tinted($x.0, $x.1, texture, tc, color, blend, true, true)
			}
			$crate::prelude::Atch::FoggyComplex(texture, tc, gradient, blend) => {
				window.shape_complex($x.0, $x.1, texture, tc, gradient, blend, true, true)
			}
		} ),* ]
	} }
}

/*/// Macro to load textures from files for the window.
///
/// The first texture file listed is indexed 0, and each subsequent texture
/// increases by 1.  See: [`sprites!()`](macro.sprites.html) for example.
#[macro_export] macro_rules! models {
	($window:expr, $( $x:expr ),*) => { {
		[ $( {
			$window.model($x.0, $x.1)
		} ),* ]
	} }
}*/

/// Macro to load textures from files for the window.
///
/// The first texture file listed is indexed 0, and each subsequent texture
/// increases by 1.  See: [`sprites!()`](macro.sprites.html) for example.
#[macro_export] macro_rules! textures {
	($window:expr, $decode:expr, $( $x:expr ),*) => { {
		[ $( {
			let mut video = $decode(include_bytes!($x), $crate::Srgba)
				.unwrap();
			let wh = video.wh();
			let px = video.pop().unwrap();

			$window.texture(wh, &px)
		} ),* ]
	} }
}
