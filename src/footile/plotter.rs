// plotter.rs      Vector path plotter.
//
// Copyright (c) 2017-2018  Douglas P Lau
//
use footile::fig::{Fig, FigDir, Vid};
use footile::path::{FillRule, JoinStyle, PathOp};
use footile::geom::{Transform, Vec2, Vec2w, float_lerp, intersection};
use footile::mask::Mask;

/// Plotter for 2D vector paths.
///
/// This is a software vector rasterizer featuring high quality anti-aliasing.
/// Paths can be created using [PathBuilder](struct.PathBuilder.html).
/// When a plot is complete, a [Mask](struct.Mask.html) of the result can be
/// used to composite a [Raster](struct.Raster.html).
///
/// # Example
/// ```
/// use footile::{PathBuilder, Plotter};
/// let path = PathBuilder::new().pen_width(3f32)
///                        .move_to(50f32, 34f32)
///                        .cubic_to(4f32, 16f32, 16f32, 28f32, 0f32, 32f32)
///                        .cubic_to(-16f32, -4f32, -4f32, -16f32, 0f32, -32f32)
///                        .close().build();
/// let mut p = Plotter::new(100, 100);
/// p.add_ops(&path);
/// p.stroke();
/// ```
pub struct Plotter {
    fig        : Fig,           // drawing fig
    sfig       : Fig,           // stroking fig
    mask       : Mask,          // image mask
    sgn_area   : Vec<i16>,      // signed area buffer
    pen        : Vec2w,         // current pen position and width
    transform  : Transform,     // user to pixel affine transform
    tol_sq     : f32,           // curve decomposition tolerance squared
    s_width    : f32,           // current stroke width
    join_style : JoinStyle,     // current join style
}

impl Plotter {
    /// Create a new plotter.
    ///
    /// * `width` Width in pixels.
    /// * `height` Height in pixels.
    pub fn new(width: u32, height: u32) -> Plotter {
        let tol = 0.3f32;
        let w = if width > 0 { width } else { 100 };
        let h = if height > 0 { height } else { 100 };
        let len = w as usize;
        // Capacity must be 8-element multiple (for SIMD)
        let cap = ((len + 7) >> 3) << 3;
        let mut sgn_area = vec![0i16; cap];
        // Remove excess elements
        for _ in 0..cap-len { sgn_area.pop(); };
        Plotter {
            fig        : Fig::new(),
            sfig       : Fig::new(),
            mask       : Mask::new(w, h),
            sgn_area   : sgn_area,
            pen        : Vec2w::new(0f32, 0f32, 1f32),
            transform  : Transform::new(),
            tol_sq     : tol * tol,
            s_width    : 1f32,
            join_style : JoinStyle::Miter(4f32),
        }
    }
    /// Get width in pixels.
    pub fn width(&self) -> u32 {
        self.mask.width()
    }
    /// Get height in pixels.
    pub fn height(&self) -> u32 {
        self.mask.height()
    }
    /// Reset path and pen.
    pub fn reset(&mut self) -> &mut Self {
        self.fig.reset();
        self.sfig.reset();
        self.pen = Vec2w::new(0f32, 0f32, self.s_width);
        self
    }
    /// Clear the mask.
    pub fn clear(&mut self) -> &mut Self {
        self.mask.clear();
        self
    }
    /// Set tolerance threshold for curve decomposition.
    pub fn set_tolerance(&mut self, t: f32) -> &mut Self {
        let tol = t.max(0.01f32);
        self.tol_sq = tol * tol;
        self
    }
    /// Set the transform.
    pub fn set_transform(&mut self, t: Transform) -> &mut Self {
        self.transform = t;
        self
    }
    /// Set pen stroke width.
    ///
    /// All subsequent path points will be affected, until the stroke width
    /// is changed again.
    ///
    /// * `width` Pen stroke width.
    fn pen_width(&mut self, width: f32) {
        self.s_width = width;
    }
    /// Set stroke join style.
    ///
    /// * `js` Join style.
    pub fn set_join(&mut self, js: JoinStyle) -> &mut Self {
        self.join_style = js;
        self
    }
    /// Move the pen.
    fn move_pen(&mut self, p: Vec2w) {
        self.pen = p;
    }
    /// Transform a point.
    fn transform_point(&self, p: Vec2w) -> Vec2w {
        let pt = self.transform * p.v;
        Vec2w::new(pt.x, pt.y, p.w)
    }
    /// Add a series of ops.
    pub fn add_ops<'a, T>(&mut self, ops: T)
        where T: IntoIterator<Item=&'a PathOp>
    {
        for op in ops {
            self.add_op(op);
        }
    }
    /// Add a path operation.
    fn add_op(&mut self, op: &PathOp) {
        match op {
            &PathOp::Close()                 => self.close(),
            &PathOp::Move(bx, by)            => self.move_to(bx, by),
            &PathOp::Line(bx, by)            => self.line_to(bx, by),
            &PathOp::Quad(bx, by, cx, cy)    => self.quad_to(bx, by, cx, cy),
            &PathOp::Cubic(bx,by,cx,cy,dx,dy)=>self.cubic_to(bx,by,cx,cy,dx,dy),
            &PathOp::PenWidth(w)             => self.pen_width(w),
        };
    }
    /// Close current sub-path and move pen to origin.
    fn close(&mut self) {
        self.fig.close(true);
        self.pen = Vec2w::new(0f32, 0f32, self.s_width);
    }
    /// Move pen to a point.
    ///
    /// * `bx` X-position of point.
    /// * `by` Y-position of point.
    fn move_to(&mut self, bx: f32, by: f32) {
        let p = Vec2w::new(bx, by, self.s_width);
        self.fig.close(false);
        let b = self.transform_point(p);
        self.fig.add_point(b);
        self.move_pen(p);
    }
    /// Add a line from pen to a point.
    ///
    /// * `bx` X-position of end point.
    /// * `by` Y-position of end point.
    fn line_to(&mut self, bx: f32, by: f32) {
        let p = Vec2w::new(bx, by, self.s_width);
        let b = self.transform_point(p);
        self.fig.add_point(b);
        self.move_pen(p);
    }
    /// Add a quadratic bézier spline.
    ///
    /// The points are A (current pen position), B (control point), and C
    /// (spline end point).
    ///
    /// * `bx` X-position of control point.
    /// * `by` Y-position of control point.
    /// * `cx` X-position of end point.
    /// * `cy` Y-position of end point.
    fn quad_to(&mut self, bx: f32, by: f32, cx: f32, cy: f32) {
        let pen = self.pen;
        let bb = Vec2w::new(bx, by, (pen.w + self.s_width) / 2f32);
        let cc = Vec2w::new(cx, cy, self.s_width);
        let a = self.transform_point(pen);
        let b = self.transform_point(bb);
        let c = self.transform_point(cc);
        self.quad_to_tran(a, b, c);
        self.move_pen(cc);
    }
    /// Add a quadratic bézier spline.
    ///
    /// The spline is decomposed into a series of lines using the DeCastlejau
    /// method.
    fn quad_to_tran(&mut self, a: Vec2w, b: Vec2w, c: Vec2w) {
        let ab    = a.midpoint(b);
        let bc    = b.midpoint(c);
        let ab_bc = ab.midpoint(bc);
        let ac    = a.midpoint(c);
        if self.is_within_tolerance(ab_bc, ac) {
            self.fig.add_point(c);
        } else {
            self.quad_to_tran(a, ab, ab_bc);
            self.quad_to_tran(ab_bc, bc, c);
        }
    }
    /// Check if two points are within tolerance threshold.
    fn is_within_tolerance(&self, a: Vec2w, b: Vec2w) -> bool {
        self.is_within_tolerance2(a.v, b.v)
    }
    /// Check if two points are within tolerance threshold.
    fn is_within_tolerance2(&self, a: Vec2, b: Vec2) -> bool {
        assert!(self.tol_sq > 0f32);
        a.dist_sq(b) <= self.tol_sq
    }
    /// Add a cubic bézier spline.
    ///
    /// The points are A (current pen position), B (first control point), C
    /// (second control point) and D (spline end point).
    ///
    /// * `bx` X-position of first control point.
    /// * `by` Y-position of first control point.
    /// * `cx` X-position of second control point.
    /// * `cy` Y-position of second control point.
    /// * `dx` X-position of end point.
    /// * `dy` Y-position of end point.
    fn cubic_to(&mut self, bx: f32, by: f32, cx: f32, cy: f32, dx: f32, dy:f32){
        let pen = self.pen;
        let bw = float_lerp(pen.w, self.s_width, 1f32 / 3f32);
        let cw = float_lerp(pen.w, self.s_width, 2f32 / 3f32);
        let bb = Vec2w::new(bx, by, bw);
        let cc = Vec2w::new(cx, cy, cw);
        let dd = Vec2w::new(dx, dy, self.s_width);
        let a = self.transform_point(pen);
        let b = self.transform_point(bb);
        let c = self.transform_point(cc);
        let d = self.transform_point(dd);
        self.cubic_to_tran(a, b, c, d);
        self.move_pen(dd);
    }
    /// Add a cubic bézier spline.
    ///
    /// The spline is decomposed into a series of lines using the DeCastlejau
    /// method.
    fn cubic_to_tran(&mut self, a: Vec2w, b: Vec2w, c: Vec2w, d: Vec2w) {
        let ab    = a.midpoint(b);
        let bc    = b.midpoint(c);
        let cd    = c.midpoint(d);
        let ab_bc = ab.midpoint(bc);
        let bc_cd = bc.midpoint(cd);
        let e     = ab_bc.midpoint(bc_cd);
        let ad    = a.midpoint(d);
        if self.is_within_tolerance(e, ad) {
            self.fig.add_point(d);
        } else {
            self.cubic_to_tran(a, ab, ab_bc, e);
            self.cubic_to_tran(e, bc_cd, cd, d);
        }
    }
    /// Fill path onto the mask.
    ///
    /// * `rule` Fill rule.
    pub fn fill(&mut self, rule: FillRule) -> &mut Self {
        self.fig.fill(&mut self.mask, &mut self.sgn_area[..], rule);
        self
    }
    /// Stroke path onto the mask.
    pub fn stroke(&mut self) -> &mut Self {
        let n_subs = self.fig.sub_count();
        for i in 0..n_subs {
            self.stroke_sub(i);
        }
        self.sfig.fill(&mut self.mask, &mut self.sgn_area[..],
            FillRule::NonZero);
        self
    }
    /// Stroke one sub-figure.
    fn stroke_sub(&mut self, i: usize) {
        if self.fig.sub_points(i) > 0 {
            let start = self.fig.sub_start(i);
            let end = self.fig.sub_end(i);
            let joined = self.fig.sub_joined(i);
            self.stroke_side(i, start, FigDir::Forward);
            if joined {
                self.sfig.close(true);
            }
            self.stroke_side(i, end, FigDir::Reverse);
            self.sfig.close(joined);
        }
    }
    /// Stroke one side of a sub-figure to another figure.
    fn stroke_side(&mut self, i: usize, start: Vid, dir: FigDir) {
        let mut xr: Option<(Vec2, Vec2)> = None;
        let mut v0 = start;
        let mut v1 = self.fig.next(v0, dir);
        let joined = self.fig.sub_joined(i);
        for _ in 0..self.fig.sub_points(i) {
            let p0 = self.fig.get_point(v0);
            let p1 = self.fig.get_point(v1);
            let bounds = self.stroke_offset(p0, p1);
            let (pr0, pr1) = bounds;
            if let Some((xr0, xr1)) = xr {
                self.stroke_join(p0, xr0, xr1, pr0, pr1);
            } else if !joined {
                self.stroke_point(pr0);
            }
            xr = Some(bounds);
            v0 = v1;
            v1 = self.fig.next(v1, dir);
        }
        if !joined {
            if let Some((_, xr1)) = xr {
                self.stroke_point(xr1);
            }
        }
    }
    /// Offset segment by half stroke width.
    ///
    /// * `p0` First point.
    /// * `p1` Second point.
    fn stroke_offset(&self, p0: Vec2w, p1: Vec2w) -> (Vec2, Vec2) {
        // FIXME: scale offset to allow user units as well as pixel units
        let pp0 = p0.v;
        let pp1 = p1.v;
        let vr = (pp1 - pp0).right().normalize();
        let pr0 = pp0 + vr * (p0.w / 2f32);
        let pr1 = pp1 + vr * (p1.w / 2f32);
        (pr0, pr1)
    }
    /// Add a point to stroke figure.
    fn stroke_point(&mut self, pt: Vec2) {
        self.sfig.add_point(Vec2w::new(pt.x, pt.y, 1f32));
    }
    /// Add a stroke join.
    ///
    /// * `p` Join point (with stroke width).
    /// * `a0` First point of A segment.
    /// * `a1` Second point of A segment.
    /// * `b0` First point of B segment.
    /// * `b1` Second point of B segment.
    fn stroke_join(&mut self, p: Vec2w, a0: Vec2, a1: Vec2, b0: Vec2, b1: Vec2){
        match self.join_style {
            JoinStyle::Miter(ml) => self.stroke_miter(a0, a1, b0, b1, ml),
            JoinStyle::Bevel     => self.stroke_bevel(a1, b0),
            JoinStyle::Round     => self.stroke_round(p, a0, a1, b0, b1),
        }
    }
    /// Add a miter join.
    fn stroke_miter(&mut self, a0: Vec2, a1: Vec2, b0: Vec2, b1: Vec2, ml: f32){
        // formula: miter_length / stroke_width = 1 / sin ( theta / 2 )
        //      so: stroke_width / miter_length = sin ( theta / 2 )
        if ml > 0f32 {
            // Minimum stroke:miter ratio
            let sm_min = 1f32 / ml;
            let th = (a1 - a0).angle_rel(b0 - b1);
            let sm = (th / 2f32).sin().abs();
            if sm >= sm_min && sm < 1f32 {
                // Calculate miter point
                if let Some(xp) = intersection(a0, a1, b0, b1) {
                    self.stroke_point(xp);
                    return;
                }
            }
        }
        self.stroke_bevel(a1, b0);
    }
    /// Add a bevel join.
    fn stroke_bevel(&mut self, a1: Vec2, b0: Vec2) {
        self.stroke_point(a1);
        self.stroke_point(b0);
    }
    /// Add a round join.
    ///
    /// * `p` Join point (with stroke width).
    /// * `a1` Second point of A segment.
    /// * `b0` First point of B segment.
    fn stroke_round(&mut self, p: Vec2w, a0: Vec2, a1: Vec2, b0: Vec2, b1: Vec2){
        let th = (a1 - a0).angle_rel(b0 - b1);
        if th <= 0f32 {
            self.stroke_bevel(a1, b0);
        } else {
            self.stroke_point(a1);
            self.stroke_arc(p, a1, b0);
        }
    }
    /// Add a stroke arc.
    fn stroke_arc(&mut self, p: Vec2w, a: Vec2, b: Vec2) {
        let p2 = p.v;
        let vr = (b - a).right().normalize();
        let c = p2 + vr * (p.w / 2f32);
        let ab = a.midpoint(b);
        if self.is_within_tolerance2(c, ab) {
            self.stroke_point(b);
        } else {
            self.stroke_arc(p, a, c);
            self.stroke_arc(p, c, b);
        }
    }
    /// Get the mask.
    pub fn mask(&self) -> &Mask {
        &self.mask
    }
}
