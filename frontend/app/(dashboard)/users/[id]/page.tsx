"use client"

import { useEffect, useState } from "react"
import { useParams } from "next/navigation"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { ArrowLeft, UserCog, History, Shield, AlertTriangle, Loader2 } from "lucide-react"
import Link from "next/link"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { Alert, AlertDescription } from "@/components/ui/alert"
import Cookies from "js-cookie"

interface Operator {
  id: number
  name: string
  fingerprint_id: number
  role: string
  email?: string
  phone?: string
  status: string
  created_at: string
}

export default function OperatorDetailPage() {
  // Menggunakan useParams hook untuk mendapatkan dynamic route parameters
  const params = useParams<{ id: string }>()
  const operatorId = params?.id

  const [operator, setOperator] = useState<Operator | null>(null)
  const [loading, setLoading] = useState(true)
  const [updating, setUpdating] = useState(false)
  const [error, setError] = useState("")
  const [success, setSuccess] = useState("")

  useEffect(() => {
    if (operatorId) {
      fetchOperator()
    }
  }, [operatorId])

  const fetchOperator = async () => {
    if (!operatorId) return

    try {
      const token = Cookies.get('access_token')
      const response = await fetch(`http://localhost:8000/operators/${operatorId}`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      })

      if (response.ok) {
        const data = await response.json()
        setOperator(data)
      } else {
        setError('Failed to fetch operator details')
      }
    } catch (error) {
      setError('Error fetching operator details')
    } finally {
      setLoading(false)
    }
  }

  const handleUpdate = async (updateData: Partial<Operator>) => {
    if (!operatorId) return

    setUpdating(true)
    setError("")
    setSuccess("")

    try {
      const token = Cookies.get('access_token')
      const response = await fetch(`http://localhost:8000/operators/${operatorId}`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify(updateData)
      })

      if (response.ok) {
        const updatedOperator = await response.json()
        setOperator(updatedOperator)
        setSuccess('Operator updated successfully')
        
        // Clear success message after 3 seconds
        setTimeout(() => setSuccess(""), 3000)
      } else {
        const errorData = await response.json()
        setError(errorData.detail || 'Failed to update operator')
      }
    } catch (error) {
      setError('Network error. Please try again.')
    } finally {
      setUpdating(false)
    }
  }

  const handleDelete = async () => {
    if (!operatorId) return
    if (!confirm('Are you sure you want to delete this operator?')) return

    try {
      const token = Cookies.get('access_token')
      const response = await fetch(`http://localhost:8000/operators/${operatorId}`, {
        method: 'DELETE',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      })

      if (response.ok) {
        // Redirect to users list
        window.location.href = '/users'
      } else {
        setError('Failed to delete operator')
      }
    } catch (error) {
      setError('Error deleting operator')
    }
  }

  // Loading state
  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <Loader2 className="h-8 w-8 animate-spin" />
      </div>
    )
  }

  // Error state - operator not found
  if (!operator) {
    return (
      <div className="flex flex-col items-center justify-center h-64">
        <p className="text-muted-foreground">Operator not found</p>
        <Button asChild className="mt-4">
          <Link href="/operators">Back to Operators</Link>
        </Button>
      </div>
    )
  }

  return (
    <div className="flex flex-col gap-4">
      <div className="flex items-center gap-2">
        <Button variant="ghost" size="icon" asChild>
          <Link href="/operators">
            <ArrowLeft className="h-4 w-4" />
          </Link>
        </Button>
        <div>
          <h1 className="text-3xl font-bold tracking-tight">
            {operator.name}
          </h1>
          <p className="text-muted-foreground">Operator profile and account management</p>
        </div>
      </div>

      {error && (
        <Alert variant="destructive">
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      )}

      {success && (
        <Alert>
          <AlertDescription>{success}</AlertDescription>
        </Alert>
      )}

      <Tabs defaultValue="profile" className="space-y-4">
        <TabsList>
          <TabsTrigger value="profile">
            <UserCog className="h-4 w-4 mr-2" />
            Profile
          </TabsTrigger>
          <TabsTrigger value="attendance">
            <History className="h-4 w-4 mr-2" />
            Attendance
          </TabsTrigger>
          <TabsTrigger value="security">
            <Shield className="h-4 w-4 mr-2" />
            Security
          </TabsTrigger>
        </TabsList>

        <TabsContent value="profile">
          <Card>
            <CardHeader>
              <CardTitle>Profile Information</CardTitle>
              <CardDescription>View and update operator profile details</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="space-y-2">
                  <Label htmlFor="name">Full Name</Label>
                  <Input 
                    id="name" 
                    defaultValue={operator.name}
                    onBlur={(e) => {
                      if (e.target.value !== operator.name) {
                        handleUpdate({ name: e.target.value })
                      }
                    }}
                    disabled={updating}
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="fingerprint_id">Fingerprint ID</Label>
                  <Input 
                    id="fingerprint_id" 
                    value={operator.fingerprint_id} 
                    disabled
                    className="bg-muted"
                  />
                </div>
              </div>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="space-y-2">
                  <Label htmlFor="email">Email</Label>
                  <Input 
                    id="email" 
                    type="email" 
                    defaultValue={operator.email || ""}
                    onBlur={(e) => {
                      if (e.target.value !== operator.email) {
                        handleUpdate({ email: e.target.value || null })
                      }
                    }}
                    disabled={updating}
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="phone">Phone Number</Label>
                  <Input 
                    id="phone" 
                    defaultValue={operator.phone || ""}
                    onBlur={(e) => {
                      if (e.target.value !== operator.phone) {
                        handleUpdate({ phone: e.target.value || null })
                      }
                    }}
                    disabled={updating}
                  />
                </div>
              </div>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="space-y-2">
                  <Label htmlFor="role">Role</Label>
                  <Select 
                    defaultValue={operator.role}
                    onValueChange={(value) => handleUpdate({ role: value })}
                    disabled={updating}
                  >
                    <SelectTrigger id="role">
                      <SelectValue placeholder="Select role" />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="operator">Operator</SelectItem>
                      <SelectItem value="admin">Admin</SelectItem>
                    </SelectContent>
                  </Select>
                </div>
                <div className="space-y-2">
                  <Label htmlFor="status">Account Status</Label>
                  <Select 
                    defaultValue={operator.status}
                    onValueChange={(value) => handleUpdate({ status: value })}
                    disabled={updating}
                  >
                    <SelectTrigger id="status">
                      <SelectValue placeholder="Select status" />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="Active">Active</SelectItem>
                      <SelectItem value="Pending">Pending</SelectItem>
                      <SelectItem value="Suspended">Suspended</SelectItem>
                    </SelectContent>
                  </Select>
                </div>
              </div>
            </CardContent>
            <CardFooter className="flex justify-between">
              <Button 
                variant="destructive" 
                onClick={handleDelete}
                className="gap-2"
                disabled={updating}
              >
                <AlertTriangle className="h-4 w-4" />
                Delete Operator
              </Button>
              <div className="text-sm text-muted-foreground">
                Created: {new Date(operator.created_at).toLocaleDateString()}
              </div>
            </CardFooter>
          </Card>
        </TabsContent>

        <TabsContent value="attendance">
          <Card>
            <CardHeader>
              <CardTitle>Attendance History</CardTitle>
              <CardDescription>View attendance logs for this operator</CardDescription>
            </CardHeader>
            <CardContent>
              <p className="text-muted-foreground">Attendance logs will be displayed here</p>
            </CardContent>
          </Card>
        </TabsContent>

        <TabsContent value="security">
          <Card>
            <CardHeader>
              <CardTitle>Security Settings</CardTitle>
              <CardDescription>Manage operator security and access</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="space-y-2">
                <Label>Reset Password</Label>
                <div className="flex gap-2">
                  <Button variant="outline" className="w-full" disabled={updating}>
                    Generate New Password
                  </Button>
                </div>
                <p className="text-sm text-muted-foreground">
                  This will generate a new temporary password for the operator
                </p>
              </div>
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>
    </div>
  )
}
