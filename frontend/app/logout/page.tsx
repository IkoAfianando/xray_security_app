// app/logout/page.tsx
"use client"

import { useEffect, useState } from "react"
import { useRouter } from "next/navigation"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Loader2 } from "lucide-react"

export default function LogoutPage() {
  const [isLoggingOut, setIsLoggingOut] = useState(true)
  const [message, setMessage] = useState("Logging you out...")
  const router = useRouter()

  useEffect(() => {
    const performLogout = async () => {
      try {
        const token = localStorage.getItem('access_token')
        
        if (token) {
          // Call backend logout endpoint
          const response = await fetch('http://localhost:8000/logout', {
            method: 'POST',
            headers: {
              'Authorization': `Bearer ${token}`,
              'Content-Type': 'application/json'
            }
          })

          if (response.ok) {
            setMessage("Successfully logged out")
          } else {
            setMessage("Logout completed")
          }
        }

        // Clear all stored data
        localStorage.removeItem('access_token')
        localStorage.removeItem('user')
        
        // Clear any cookies if you're using them
        document.cookie = 'access_token=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;'
        
        // Wait a moment to show the message
        setTimeout(() => {
          setIsLoggingOut(false)
          // Redirect to login page
          router.push('/login')
        }, 2000)

      } catch (error) {
        console.error('Logout error:', error)
        setMessage("Logout completed")
        
        // Clear storage anyway
        localStorage.removeItem('access_token')
        localStorage.removeItem('user')
        
        setTimeout(() => {
          setIsLoggingOut(false)
          router.push('/login')
        }, 2000)
      }
    }

    performLogout()
  }, [router])

  return (
    <div className="min-h-screen flex items-center justify-center bg-gray-50">
      <Card className="w-full max-w-md">
        <CardHeader className="text-center">
          <CardTitle className="text-2xl">Logging Out</CardTitle>
          <CardDescription>Please wait while we log you out safely</CardDescription>
        </CardHeader>
        <CardContent className="flex flex-col items-center space-y-4">
          {isLoggingOut && (
            <Loader2 className="h-8 w-8 animate-spin text-primary" />
          )}
          <p className="text-center text-muted-foreground">{message}</p>
        </CardContent>
      </Card>
    </div>
  )
}
