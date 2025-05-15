import type { Metadata } from "next";
import { Inter } from "next/font/google";
// Adjusted import path for globals.css
import '../styles/globals.css';

const inter = Inter({ subsets: ["latin"] });

export const metadata: Metadata = {
  title: "X-Ray Machine Security",
  description: "Enhanced Security System for X-Ray Machines",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className={inter.className}>{children}</body>
    </html>
  );
}
