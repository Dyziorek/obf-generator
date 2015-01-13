// DirectXTK MakeSpriteFont tool
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Runtime.InteropServices;

namespace MakeSpriteFont
{
    // Uses System.Drawing (aka GDI+) to rasterize TrueType fonts into a series of glyph bitmaps.
    public class TrueTypeImporter : IFontImporter
    {
        // Properties hold the imported font data.
        public IEnumerable<Glyph> Glyphs { get; private set; }

        public float LineSpacing { get; private set; }


        // Size of the temp surface used for GDI+ rasterization.
        const int MaxGlyphSize = 1024;


        public void Import(CommandLineOptions options)
        {
            // Create a bunch of GDI+ objects.
            using (Font font = CreateFont(options))
            using (Brush brush = new SolidBrush(Color.White))
            using (StringFormat stringFormat = new StringFormat(StringFormatFlags.NoFontFallback))
            using (Bitmap bitmap = new Bitmap(MaxGlyphSize, MaxGlyphSize, PixelFormat.Format32bppArgb))
            using (Graphics graphics = Graphics.FromImage(bitmap))
            {
                graphics.PixelOffsetMode = options.Sharp ? PixelOffsetMode.None : PixelOffsetMode.HighQuality;
                graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
                graphics.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;

                // Which characters do we want to include?
                var characters = CharacterRegion.Flatten(options.CharacterRegions);

                var glyphList = new List<Glyph>();

                // Rasterize each character in turn.
                int count = 0;

                foreach (char character in characters)
                {
                    ++count;

                    if (count == 500)
                    {
                        Console.WriteLine("WARNING: capturing a large font. This may take a long time to complete and could result in too large a texture. Consider using a smaller set of character regions.");
                        Console.Write(".");
                    }
                    else if ((count % 500) == 0)
                    {
                        Console.Write(".");
                    }
                    Glyph glyph = null;
                    if (options.GenerateDistance)
                    {
                        glyph = ImportGlyph(character, font, brush, stringFormat, bitmap, graphics, options.Downscale);
                        GenerateDistances(glyph, options.Downscale, options.Spread, options.DistanceColor);
                    }
                    else
                    {
                        glyph = ImportGlyph(character, font, brush, stringFormat, bitmap, graphics);
                    }
                    glyphList.Add(glyph);
                }

                if (count > 500)
                {
                    Console.WriteLine();
                }

                Glyphs = glyphList;

                // Store the font height.
                LineSpacing = font.GetHeight();
            }
        }

        private void GenerateDistances(Glyph glyph, int downscale, float spread, Color distanceColor)
        {
            Bitmap fontData = glyph.Bitmap;
            int inWidth = fontData.Width;
		    int inHeight = fontData.Height;
		    int outWidth = inWidth / downscale;
		    int outHeight = inHeight / downscale;
		    Bitmap outImage = new Bitmap(outWidth, outHeight);
            bool[,] bitmapMask = new bool[inWidth, inHeight];
            fontData.Save(@"D:\osmData\inms.bmp");
            using (BitmapUtils.PixelAccessor inputAcces = new BitmapUtils.PixelAccessor(fontData, ImageLockMode.ReadOnly))
            {
                
                for (int x = 0; x < inWidth; ++x)
                {
                    for (int y = 0; y < inHeight; ++y)
                    {
                        bitmapMask[x, y] = isInside(inputAcces[x, y]);
			        }
		        }
            }

            using (BitmapUtils.PixelAccessor accessor = new BitmapUtils.PixelAccessor(outImage, ImageLockMode.WriteOnly))
            {
                for (int x = 0; x < outWidth; ++x)
		        {
                    for (int y = 0; y < outHeight; ++y)
			        {
				        int centerX = (x * downscale) + (downscale / 2);
				        int centerY = (y * downscale) + (downscale / 2);
                        float signedDistance = findSignedDistance(centerX, centerY, ref bitmapMask, inWidth, inHeight, spread);
                        accessor[x, y] = distanceToRGB(signedDistance, spread, distanceColor);
			        }
		        }
            }

            glyph.Subrect = new Rectangle(0, 0, outWidth, outHeight);
            glyph.Bitmap = outImage;
            outImage.Save(@"D:\osmData\ms.bmp");
        }

        private Color distanceToRGB(float signedDistance, float spread, Color color)
        {
            float alpha = 0.5f + 0.5f * (signedDistance / spread);
            alpha = Math.Min(1, Math.Max(0, alpha)); // compensate for rounding errors
            int alphaByte = (int)(alpha * 0xFF); // no unsigned byte in Java :(
            return  Color.FromArgb((alphaByte << 24) | (color.ToArgb() & 0xFFFFFF));
        }

        private float findSignedDistance(int centerX, int centerY, ref bool[,] bitmapMask, int width, int height, float spread)
        {
		   bool baseMask = bitmapMask[centerX,centerY];
		
		    int delta = (int) Math.Ceiling(spread);
		    int startX = Math.Max(0, centerX - delta);
		    int endX  = Math.Min(width - 1, centerX + delta);
		    int startY = Math.Max(0, centerY - delta);
		    int endY = Math.Min(height - 1, centerY + delta);

		    int closestSquareDist = delta * delta;
		
		    for (int x = startX; x <= endX; ++x)
		    {
			    for (int y = startY; y <= endY; ++y)
			    {
                    if (baseMask != bitmapMask[x,y])
				    {
					    int squareDist = squareDistCalc(centerX, centerY, x, y);
					    if (squareDist < closestSquareDist)
					    {
						    closestSquareDist = squareDist;
					    }
				    }
			    }
		    }
		
		float closestDist = (float) Math.Sqrt(closestSquareDist);
		return (baseMask ? 1 : -1) * Math.Min(closestDist, spread);
        }

        private int squareDistCalc(int centerX, int centerY, int x, int y)
        {
            int dx = centerX - x;
            int dy = centerY - y;
		    return dx*dx + dy*dy;
        }

        private bool isInside(Color color)
        {
            int argbColor = color.ToArgb();
            return (argbColor & 0x808080) != 0 && (argbColor & 0x80000000) != 0;
        }

        

        // Attempts to instantiate the requested GDI+ font object.
        static Font CreateFont(CommandLineOptions options)
        {
            Font font = new Font(options.SourceFont, PointsToPixels(options.FontSize), options.FontStyle, GraphicsUnit.Pixel);

            try
            {
                // The font constructor automatically substitutes fonts if it can't find the one requested.
                // But we prefer the caller to know if anything is wrong with their data. A simple string compare
                // isn't sufficient because some fonts (eg. MS Mincho) change names depending on the locale.

                // Early out: in most cases the name will match the current or invariant culture.
                if (options.SourceFont.Equals(font.FontFamily.GetName(CultureInfo.CurrentCulture.LCID), StringComparison.OrdinalIgnoreCase) ||
                    options.SourceFont.Equals(font.FontFamily.GetName(CultureInfo.InvariantCulture.LCID), StringComparison.OrdinalIgnoreCase))
                {
                    return font;
                }

                // Check the font name in every culture.
                foreach (CultureInfo culture in CultureInfo.GetCultures(CultureTypes.SpecificCultures))
                {
                    if (options.SourceFont.Equals(font.FontFamily.GetName(culture.LCID), StringComparison.OrdinalIgnoreCase))
                    {
                        return font;
                    }
                }

                // A font substitution must have occurred.
                throw new Exception(string.Format("Can't find font '{0}'.", options.SourceFont));
            }
            catch
            {
                font.Dispose();
                throw;
            }
        }


        // Converts a font size from points to pixels. Can't just let GDI+ do this for us,
        // because we want identical results on every machine regardless of system DPI settings.
        static float PointsToPixels(float points)
        {
            return points * 96 / 72;
        }

        static Glyph ImportGlyph(char character, Font font, Brush brush, StringFormat stringFormat, Bitmap bitmap, Graphics graphics, int upscale)
        {
            float upscaler = Convert.ToSingle(upscale);
            string characterString = character.ToString();

            // Measure the size of this character.
            SizeF size = graphics.MeasureString(characterString, font, Point.Empty, stringFormat);
            size.Height *= upscaler;
            size.Width *= upscaler;

            int characterWidth = (int)Math.Ceiling(size.Width);
            int characterHeight = (int)Math.Ceiling(size.Height);

            // Pad to make sure we capture any overhangs (negative ABC spacing, etc.)
            int padWidth = characterWidth;
            int padHeight = characterHeight / 2;

            int bitmapWidth = characterWidth + padWidth * 2;
            int bitmapHeight = characterHeight + padHeight * 2;

            if (bitmapWidth > MaxGlyphSize || bitmapHeight > MaxGlyphSize)
                throw new Exception("Excessively large glyph won't fit in my lazily implemented fixed size temp surface.");

            GraphicsPath pathTrans = new GraphicsPath();
            pathTrans.

            // Render the character.
            graphics.Clear(Color.White);
            graphics.ScaleTransform(upscaler, upscaler, MatrixOrder.Append);
            graphics.DrawString(characterString, font, brush, padWidth, padHeight, stringFormat);
            graphics.Flush();

            // Clone the newly rendered image.
            Bitmap glyphBitmap = bitmap.Clone(new Rectangle(0, 0, bitmapWidth, bitmapHeight), PixelFormat.Format32bppArgb);
            bitmap.Save(@"D:\osmData\fntinms.bmp");
            glyphBitmap.Save(@"D:\osmData\fntoms.bmp");
            BitmapUtils.ConvertGreyToAlpha(glyphBitmap);

            // Query its ABC spacing.
            float? abc = GetCharacterWidth(character, font, graphics);

            // Construct the output Glyph object.
            return new Glyph(character, glyphBitmap)
            {
                XOffset = -padWidth,
                XAdvance = abc.HasValue ? padWidth - bitmapWidth + abc.Value : -padWidth,
                YOffset = -padHeight,
            };

        }

        // Rasterizes a single character glyph.
        static Glyph ImportGlyph(char character, Font font, Brush brush, StringFormat stringFormat, Bitmap bitmap, Graphics graphics)
        {
            string characterString = character.ToString();

            // Measure the size of this character.
            SizeF size = graphics.MeasureString(characterString, font, Point.Empty, stringFormat);

            int characterWidth = (int)Math.Ceiling(size.Width);
            int characterHeight = (int)Math.Ceiling(size.Height);

            // Pad to make sure we capture any overhangs (negative ABC spacing, etc.)
            int padWidth = characterWidth;
            int padHeight = characterHeight / 2;

            int bitmapWidth = characterWidth + padWidth * 2;
            int bitmapHeight = characterHeight + padHeight * 2;

            if (bitmapWidth > MaxGlyphSize || bitmapHeight > MaxGlyphSize)
                throw new Exception("Excessively large glyph won't fit in my lazily implemented fixed size temp surface.");

            // Render the character.
            graphics.Clear(Color.Black);
            graphics.DrawString(characterString, font, brush, padWidth, padHeight, stringFormat);
            graphics.Flush();

            // Clone the newly rendered image.
            Bitmap glyphBitmap = bitmap.Clone(new Rectangle(0, 0, bitmapWidth, bitmapHeight), PixelFormat.Format32bppArgb);

            BitmapUtils.ConvertGreyToAlpha(glyphBitmap);

            // Query its ABC spacing.
            float? abc = GetCharacterWidth(character, font, graphics);

            // Construct the output Glyph object.
            return new Glyph(character, glyphBitmap)
            {
                XOffset = -padWidth,
                XAdvance = abc.HasValue ? padWidth - bitmapWidth + abc.Value : -padWidth,
                YOffset = -padHeight,
            };
        }


        // Queries APC spacing for the specified character.
        static float? GetCharacterWidth(char character, Font font, Graphics graphics)
        {
            // Look up the native device context and font handles.
            IntPtr hdc = graphics.GetHdc();

            try
            {
                IntPtr hFont = font.ToHfont();

                try
                {
                    // Select our font into the DC.
                    IntPtr oldFont = NativeMethods.SelectObject(hdc, hFont);

                    try
                    {
                        // Query the character spacing.
                        var result = new NativeMethods.ABCFloat[1];

                        if (NativeMethods.GetCharABCWidthsFloat(hdc, character, character, result))
                        {
                            return result[0].A + 
                                   result[0].B + 
                                   result[0].C;
                        }
                        else
                        {
                            return null;
                        }
                    }
                    finally
                    {
                        NativeMethods.SelectObject(hdc, oldFont);
                    }
                }
                finally
                {
                    NativeMethods.DeleteObject(hFont);
                }
            }
            finally
            {
                graphics.ReleaseHdc(hdc);
            }
        }


        // Interop to the native GDI GetCharABCWidthsFloat method.
        static class NativeMethods
        {
            [DllImport("gdi32.dll")]
            public static extern IntPtr SelectObject(IntPtr hdc, IntPtr hObject);

            [DllImport("gdi32.dll")]
            public static extern bool DeleteObject(IntPtr hObject);

            [DllImport("gdi32.dll", CharSet = CharSet.Unicode)]
            public static extern bool GetCharABCWidthsFloat(IntPtr hdc, uint iFirstChar, uint iLastChar, [Out] ABCFloat[] lpABCF);


            [StructLayout(LayoutKind.Sequential)]
            public struct ABCFloat
            {
                public float A;
                public float B;
                public float C;
            }
        }
    }
}
