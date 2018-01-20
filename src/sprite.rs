// Aldaron's Device Interface / Screen
// Copyright (c) 2017 Plop Grizzly, Jeron Lau <jeron.lau@plopgrizzly.com>
// Licensed under the MIT LICENSE
//
// src/sprite.rs

use Window;
use { Texture, Model, Gradient, TexCoords };
use adi_gpu::{ Shape, ShapeBuilder };
use adi_gpu;

#[must_use]
/// Sprite represents anything that is rendered onto the screen.
pub struct Sprite(Shape);

#[must_use]
/// Builder for a `Sprite`.
pub struct SpriteBuilder(ShapeBuilder, bool, bool);

/// Builder for multiple `Sprite`s.
#[must_use]
pub struct SpriteList(Vec<Sprite>, adi_gpu::Model, adi_gpu::Transform, bool,
	bool);

impl SpriteList {
	/// Create a new list of `Sprite`s.
	#[inline(always)]
	pub fn new(model: Model) -> SpriteList {
		SpriteList(vec![], model.0, adi_gpu::Transform::new(), false,
			false)
	}

	/// Set the transform.
	#[inline(always)]
	pub fn transform(self, transform: Transform) -> SpriteList {
		SpriteList(self.0, self.1, transform.0, self.3, self.4)
	}

	/// Set the model.
	#[inline(always)]
	pub fn model(self, model: Model) -> SpriteList {
		SpriteList(self.0, model.0, self.2, self.3, self.4)
	}

	/// Enable alpha blending for following sprites.
	#[inline(always)]
	pub fn alpha(self) -> Self {
		SpriteList(self.0, self.1, self.2, true, false)
	}

	/// Enable per-fragment alpha blending for following sprites.
	#[inline(always)]
	pub fn blend(self) -> Self {
		SpriteList(self.0, self.1, self.2, true, true)
	}

	/// Disable all alpha blending for following sprites.
	#[inline(always)]
	pub fn opaque(self) -> Self {
		SpriteList(self.0, self.1, self.2, false, false)
	}

	/// Create a sprite with a solid color.
	#[inline(always)]
	pub fn solid(mut self, window: &mut Window, color: [f32; 4]) -> Self {
		self.0.push(Sprite(ShapeBuilder::new(self.1).push_solid(
			&mut window.window, self.2, color, self.3, self.4)));
		self
	}

	/// Create a sprite shaded by a gradient (1 color per vertex).
	#[inline(always)]
	pub fn gradient(mut self, window: &mut Window, colors: Gradient)
		-> Self
	{
		self.0.push(Sprite(ShapeBuilder::new(self.1).push_gradient(
			&mut window.window, self.2, colors.0, self.3, self.4)));
		self
	}

	/// Create a sprite with a texture and texture coordinates.
	#[inline(always)]
	pub fn texture(mut self, window: &mut Window, texture: Texture,
		tc: TexCoords) -> Self
	{
		self.0.push(Sprite(ShapeBuilder::new(self.1).push_texture(
			&mut window.window, self.2, texture.0, tc.0, self.3,
			self.4)));
		self
	}

	/// Create a sprite with a texture, texture coordinates and alpha.
	/// Automatically Enables Alpha Blending. (no need to call `alpha()`)
	#[inline(always)]
	pub fn faded(mut self, window: &mut Window, texture: Texture,
		tc: TexCoords, alpha: f32) -> Self
	{
		self.0.push(Sprite(ShapeBuilder::new(self.1).push_faded(
			&mut window.window, self.2, texture.0, tc.0, alpha,
			self.4)));
		self
	}

	/// Create a sprite with a texture and texture coordinates and tint.
	#[inline(always)]
	pub fn tinted(mut self, window: &mut Window, texture: Texture,
		tc: TexCoords, tint: [f32; 4]) -> Self
	{
		self.0.push(Sprite(ShapeBuilder::new(self.1).push_tinted(
			&mut window.window, self.2, texture.0, tc.0, tint,
			self.3, self.4)));
		self
	}

	/// Create a sprite with a texture and texture coordinates and tint per
	/// vertex.
	#[inline(always)]
	pub fn complex(mut self, window: &mut Window, texture: Texture,
		tc: TexCoords, tint_pv: Gradient) -> Self
	{
		self.0.push(Sprite(ShapeBuilder::new(self.1).push_complex(
			&mut window.window, self.2, texture.0, tc.0, tint_pv.0,
			self.3, self.4)));
		self
	}

	/// Convert into a `Vec` of `Sprite`s
	#[inline(always)]
	pub fn to_vec(self) -> Vec<Sprite> {
		self.0
	}

	/// Convert into 1 `Sprite` if there's only 1 in the list.
	#[inline(always)]
	pub fn only(mut self) -> Sprite {
		self.0.pop().unwrap()
	}
}

/// Transform represents a transformation matrix.
pub struct Transform(adi_gpu::Transform);

impl SpriteBuilder {
	/// Create a new `SpriteBuilder`.
	#[inline(always)]
	pub fn new(vertices: Model) -> Self {
		SpriteBuilder(ShapeBuilder::new(vertices.0), false, false)
	}

	/// Enable alpha blending for this sprite.
	#[inline(always)]
	pub fn alpha(self) -> Self {
		SpriteBuilder(self.0, true, false)
	}

	/// Enable per-fragment alpha blending for this sprite.
	#[inline(always)]
	pub fn blend(self) -> Self {
		SpriteBuilder(self.0, true, true)
	}

/*	/// Create a sprite with a solid color.
	#[inline(always)]
	pub fn solid(&self, window: &mut Window, color: [f32; 4]) -> Sprite {
		Sprite(self.0.push_solid(&mut window.window, color, self.1,
			self.2))
	}

	/// Create a sprite shaded by a gradient (1 color per vertex).
	#[inline(always)]
	pub fn gradient(&self, window: &mut Window, colors: Gradient) -> Sprite {
		Sprite(self.0.push_gradient(&mut window.window, colors.0,
			self.1, self.2))
	}

	/// Create a sprite with a texture and texture coordinates.
	#[inline(always)]
	pub fn texture(&self, window: &mut Window, texture: Texture, tc: TexCoords)
		-> Sprite
	{
		Sprite(self.0.push_texture(&mut window.window, texture.0, tc.0,
			self.1, self.2))
	}

	/// Create a sprite with a texture, texture coordinates and alpha.
	/// Automatically Enables Alpha Blending. (no need to call `alpha()`)
	#[inline(always)]
	pub fn faded(&self, window: &mut Window, texture: Texture, tc: TexCoords,
		alpha: f32) -> Sprite
	{
		Sprite(self.0.push_faded(&mut window.window, texture.0, tc.0,
			alpha, self.2))
	}

	/// Create a sprite with a texture and texture coordinates and tint.
	#[inline(always)]
	pub fn tinted(&self, window: &mut Window, texture: Texture,
		tc: TexCoords, tint: [f32; 4]) -> Sprite
	{
		Sprite(self.0.push_tinted(&mut window.window, texture.0, tc.0,
			tint, self.1, self.2))
	}

	/// Create a sprite with a texture and texture coordinates and tint per
	/// vertex.
	#[inline(always)]
	pub fn complex(&self, window: &mut Window, texture: Texture,
		tc: TexCoords, tint_pv: Gradient) -> Sprite
	{
		Sprite(self.0.push_complex(&mut window.window, texture.0, tc.0,
			tint_pv.0, self.1, self.2))
	}*/
}

/*	/// Change the style of self to style for instance i.
	pub fn style(&self, window: &mut Window, i: usize, style: &Style) -> (){
		match *style {
			Style::Invisible => {
				Shape::enable(window, self.0, i, false);
			}
			Style::Texture(s, ref tx) => {
				let shader = window.shader(s);
				Shape::animate(window, self.0, i, tx, shader);
			}
			Style::Solid(s) => {
				let shader = window.shader(s);
				Shape::animate(window, self.0, i, null_mut(),
					shader);
			}
		}
	}*/

/*	/// Change the vertices of self to v.
	pub fn vertices(&mut self, window: &mut Window, v: &[f32]) -> () {
		self.0.vertices(window, self.0, v);
	}*/
//}

impl Transform {
	/// Create a transform that does nothing. ( Underneath, this is an
	/// identity matrix ).
	pub fn new() -> Transform {
		Transform (adi_gpu::Transform::new())
	}

	/// Translate self by x, y and z.
	pub fn translate(self, x:f32, y:f32, z:f32) -> Transform {
		Transform(self.0.translate(x, y, z))
	}

	/// Scale self by x, y and z.
	pub fn scale(self, x:f32, y:f32, z:f32) -> Transform {
		Transform(self.0.scale(x, y, z))
	}

	/// Rotate self by yaw, pitch and roll.
	pub fn rotate(self, yaw:f32, pitch:f32, roll:f32) -> Transform {
		Transform(self.0.rotate(yaw, pitch, roll))
	}

	/// Multiply by a projection that scales width and height by the
	/// smallest widget size. The widget is put at position pos. Position
	/// isn't affected by aspect ratio.
	pub fn auto(self, window: &mut Window, pos: (f32, f32)) -> Transform {
		let size = window.unit_size();
		let t = self.scale(size.0, size.1, 1.0)
			.translate(pos.0, pos.1, 0.0);
		t
	}

	/// Apply a TransformApply onto instance i of Sprite.
	pub fn apply(self, window: &mut Window, sprite: &mut Sprite)
		-> Transform
	{
		sprite.0.transform(&mut window.window, &self.0);

		self
	}
}
