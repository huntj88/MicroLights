/** @type {import('next').NextConfig} */
const nextConfig = {
  output: "export",
  experimental: {
    swcPlugins: [
      ["fluentui-next-appdir-directive", { paths: ["@griffel", "@fluentui"] }],
    ],
  },
};

export default nextConfig;
