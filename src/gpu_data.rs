// Copyright Jeron A. Lau 2017 - 2018.
// Dual-licensed under either the MIT License or the Boost Software License,
// Version 1.0.  (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

use awi::render as adi_gpu;
use Window;
use awi::render::*;
use Transform;

/// Macro to load models from files for the window.
///
/// The first model file listed is indexed 0, and each subsequent model
/// increases by 1.  See: [`sprites!()`](macro.sprites.html) for example.
#[macro_export] macro_rules! models {
	($window:expr, $( $x:expr ),*) => { {
		use $crate::ModelBuilder as model;
		const IDENTITY: Transform =  $crate::Transform::IDENTITY;

		let a = vec![ $( include!($x).finish($window) ),* ];

		$window.models(a);
	} }
}

/// The builder for `Model`.
pub struct ModelBuilder {
	// Final output
	vertices: Vec<f32>,
	// Build a tristrip
	ts: Vec<[f32; 4]>,
	// Final output
	colors: Vec<f32>,
	// Build a tristrip
	colors_ts: Vec<[f32; 4]>,
	// Final output
	tcs: Vec<f32>,
	// Build a tristrip
	tcs_ts: Vec<[f32; 4]>,
	//
	color: Option<[f32; 4]>,
	//
	opacity: Option<f32>,
	// A list of the fans to draw (start vertex, vertex count)
	fans: Vec<(u32, u32)>,
	//
	mat4: Transform,
}

impl ModelBuilder {
	#[doc(hidden)]
	pub fn new() -> Self {
		ModelBuilder {
			vertices: vec![],
			ts: vec![],
			colors: vec![],
			colors_ts: vec![],
			tcs: vec![],
			tcs_ts: vec![],
			color: None,
			opacity: None,
			fans: vec![],
			mat4: Transform::IDENTITY,
		}
	}

	/// Set transformation matrix
	pub fn m(mut self, mat4: Transform) -> Self {
		self.mat4 = mat4;

		self
	}

	/// Set one color for the whole model.
	pub fn c(mut self, color: [f32;4]) -> Self {
		self.color = Some(color);
		self
	}

	/// Set the opacity for the whole model.
	pub fn o(mut self, opacity: f32) -> Self {
		self.opacity = Some(opacity);
		self
	}

	/// Set the colors for the following faces.
	pub fn g(mut self, vertices: &[[f32;4]]) -> Self {
		self.colors_ts = vec![];
		self.colors_ts.extend(vertices);
		self
	}

	/// Set the texture coordinates for the following faces.
	pub fn t(mut self, vertices: &[[f32;4]]) -> Self {
		self.tcs_ts = vec![];
		self.tcs_ts.extend(vertices);
		self
	}

	/// Set the vertices for the following faces.
	pub fn v(mut self, vertices: &[[f32;4]]) -> Self {
		self.ts = vec![];
		self.ts.extend(vertices);
		self
	}

	/// Set the vertices for a double-sided face (actually 2 faces)
	pub fn d(mut self) -> Self {
		self = self.shape();
		self.ts.reverse();
		let origin = self.ts.pop().unwrap();
		self.ts.insert(0, origin);
		self = self.shape();
		self.mat4 = Transform::IDENTITY;
		self
	}

	/// Add a face to the model, this unapplies the transformation matrix.
	pub fn f(mut self) -> Self {
		self = self.shape();
		self.mat4 = Transform::IDENTITY;
		self
	}

	/// Add a shape to the model.
	pub fn shape(mut self) -> Self {
		if self.ts.len() == 0 { return self; }

		let start = self.vertices.len() / 4;
		let length = self.ts.len();

		self.fans.push((start as u32, length as u32));

		for i in &self.ts {
			let v = self.mat4.0 * Vec4::new(i[0], i[1], i[2], i[3]);

			self.vertices.push(v.x as f32);
			self.vertices.push(v.y as f32);
			self.vertices.push(v.z as f32);
			self.vertices.push(v.w as f32);
		}
		for i in &self.colors_ts {
			self.colors.push(i[0] as f32);
			self.colors.push(i[1] as f32);
			self.colors.push(i[2] as f32);
			self.colors.push(i[3] as f32);
		}
		for i in &self.tcs_ts {
			self.tcs.push(i[0] as f32);
			self.tcs.push(i[1] as f32);
			self.tcs.push(i[2] as f32);
			self.tcs.push(i[3] as f32);
		}

		self
	}

	/// Create the model
	pub fn finish(self, window: &mut Window) -> Model {
		Model(window.window.model(self.vertices.as_slice(), self.fans),
			if self.colors.is_empty() {
				None
			} else {
				assert!(self.colors.len() == self.vertices.len());
				Some(window.window.gradient(self.colors.as_slice()))
			},
			if self.tcs.is_empty() {
				None
			} else {
				assert_eq!(self.tcs.len(), self.vertices.len());
				Some(window.window.texcoords(self.tcs.as_slice()))
			}, self.color, self.opacity)
	}
}

/// A collection of indices and vertices
pub struct Model(pub(crate) adi_gpu::Model,
	pub(crate) Option<adi_gpu::Gradient>,
	pub(crate) Option<adi_gpu::TexCoords>,
	pub(crate) Option<[f32; 4]>,
	pub(crate) Option<f32>);
