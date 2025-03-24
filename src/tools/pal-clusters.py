import os
import shutil
import argparse
import numpy as np
from PIL import Image
from sklearn.cluster import KMeans

def compute_mse(original, quantized):
    """
    Compute Mean Squared Error between two images (numpy arrays).
    """
    if original.shape != quantized.shape:
        raise ValueError("Images must have the same dimensions")
    mse = np.mean((original - quantized) ** 2)
    return mse

def extract_palette_feature(quantized_image, num_colors=16):
    """
    Extract a flattened feature vector from the quantized image palette.
    Uses the getcolors() method to get (count, color) tuples.
    If the color is an integer (palette index), it converts it to an RGB tuple.
    Sorts the colors by brightness (sum of RGB).
    Pads with zeros if fewer than num_colors colors are present.
    """
    colors = quantized_image.getcolors(maxcolors=256)
    if colors is None:
        raise ValueError("Too many colors in quantized image")
    
    raw_palette = quantized_image.getpalette()
    color_list = []
    for count, col in colors:
        # Convert palette index to RGB if needed.
        if isinstance(col, int):
            rgb = tuple(raw_palette[col*3: col*3+3])
        else:
            rgb = col
        color_list.append(rgb)
    
    color_list.sort(key=lambda c: sum(c))
    
    if len(color_list) < num_colors:
        pad_length = num_colors - len(color_list)
        color_list.extend([(0, 0, 0)] * pad_length)
    elif len(color_list) > num_colors:
        color_list = color_list[:num_colors]
    
    feature = np.array(color_list).flatten()
    return feature

def process_textures(input_dir, output_dir, error_threshold, num_palette_groups=64, palette_colors=16):
    accepted_features = []
    accepted_files = []
    non_paletteable_files = []

    for filename in os.listdir(input_dir):
        if filename.lower().endswith('.tga'):
            filepath = os.path.join(input_dir, filename)
            try:
                original = Image.open(filepath).convert("RGB")
                # Quantize image to a palette of palette_colors colors.
                quantized = original.quantize(colors=palette_colors, method=Image.FASTOCTREE, dither=Image.NONE)
                quantized_rgb = quantized.convert("RGB")
                
                original_arr = np.array(original, dtype=np.float32)
                quantized_arr = np.array(quantized_rgb, dtype=np.float32)
                
                mse = compute_mse(original_arr, quantized_arr)
                
                if mse <= error_threshold:
                    feature = extract_palette_feature(quantized, num_colors=palette_colors)
                    accepted_features.append(feature)
                    accepted_files.append(filepath)
                    # print(f"File {filename} MSE={mse:.2f} -> paletteable")
                else:
                    non_paletteable_files.append(filepath)
                    # print(f"File {filename} MSE={mse:.2f} -> non-paletteable")
            except Exception as e:
                print(f"Error processing {filename}: {e}")
    
    print(f"Total textures processed: {len(accepted_files) + len(non_paletteable_files)}")
    print(f"Paletteable textures: {len(accepted_files)}")
    print(f"Non-paletteable textures: {len(non_paletteable_files)}")
    
    if accepted_features:
        features_array = np.array(accepted_features)
        kmeans = KMeans(n_clusters=num_palette_groups, random_state=42)
        cluster_labels = kmeans.fit_predict(features_array)
        
        for i in range(num_palette_groups):
            cluster_dir = os.path.join(output_dir, f"cluster_{i}")
            os.makedirs(cluster_dir, exist_ok=True)
        
        for filepath, label in zip(accepted_files, cluster_labels):
            dest_dir = os.path.join(output_dir, f"cluster_{label}")
            shutil.copy(filepath, dest_dir)
    
            cluster_filepath = filepath + ".cluster"
            with open(cluster_filepath, "w") as cluster_file:
                cluster_file.write(str(label))
    
    nonpal_dir = os.path.join(output_dir, "cluster_64")
    os.makedirs(nonpal_dir, exist_ok=True)
    for filepath in non_paletteable_files:
        shutil.copy(filepath, nonpal_dir)

        cluster_filepath = filepath + ".cluster"
        with open(cluster_filepath, "w") as cluster_file:
            cluster_file.write("64")
    
    print("Grouping complete.")

def main():
    parser = argparse.ArgumentParser(description="Group .tga textures into palette clusters and a non-paletteable group.")
    parser.add_argument("input_dir", type=str, help="Path to folder containing .tga textures")
    parser.add_argument("output_dir", type=str, help="Path to output folder for grouped textures")
    parser.add_argument("--threshold", type=float, default=500.0,
                        help="Error threshold for palette quality (default: 500.0)")
    parser.add_argument("--palette_colors", type=int, default=16,
                        help="Number of colors for palette quantization (default: 16)")
    parser.add_argument("--num_palette_groups", type=int, default=64,
                        help="Number of palette groups to form (default: 64)")
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)
    process_textures(args.input_dir, args.output_dir, args.threshold, args.num_palette_groups, args.palette_colors)

if __name__ == "__main__":
    main()
